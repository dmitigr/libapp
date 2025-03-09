// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "../../base/assert.hpp"
#include "../../jrpc/jrpc.hpp"
#include "../../net/conversions.hpp"
#include "../../uv/uv.hpp"
#include "../../wscl/wscl.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <vector>

namespace chrono = std::chrono;
namespace jrpc = dmitigr::jrpc;
namespace rajson = dmitigr::rajson;
namespace uv = dmitigr::uv;
namespace wscl = dmitigr::wscl;

std::atomic_bool is_running;

class Connection final : public wscl::Connection {
public:
  template<typename ... Types>
  Connection(const std::chrono::milliseconds rpc_response_delay_threshold,
    const std::chrono::milliseconds data_response_delay_threshold,
    Types&& ... args)
    : wscl::Connection{std::forward<Types>(args)...}
    , rpc_response_delay_threshold_{rpc_response_delay_threshold}
    , data_response_delay_threshold_{data_response_delay_threshold}
    , timepoints_(255)
  {
    DMITIGR_ASSERT(timepoints_.size() == 255);
  }

  void send(const jrpc::Request& req)
  {
    DMITIGR_ASSERT(req.id());
    const auto id = rajson::to<int>(*req.id());
    DMITIGR_ASSERT(0 < id && id < 256);
    timepoints_[id - 1] = chrono::steady_clock::now();
    send_utf8(req.to_string());
  }

  auto max_rpc_response_delay() const noexcept
  {
    return max_rpc_response_delay_;
  }

  auto max_data_response_delay() const noexcept
  {
    return max_data_response_delay_;
  }

  auto bad_rpc_response_count() const noexcept
  {
    return bad_rpc_response_count_;
  }

  auto bad_data_response_count() const noexcept
  {
    return bad_data_response_count_;
  }

private:
  using ms = chrono::milliseconds;
  ms max_rpc_response_delay_{};
  ms max_data_response_delay_{};
  ms rpc_response_delay_threshold_{};
  ms data_response_delay_threshold_{};
  int bad_rpc_response_count_{};
  int bad_data_response_count_{};
  std::vector<chrono::steady_clock::time_point> timepoints_;
  unsigned response_total_records_{};

  void handle_open() noexcept override
  {
    DMITIGR_ASSERT(is_open());
    std::clog << "Connection open!\n";
  }

  void handle_message(const std::string_view data,
    const wscl::Data_format format) noexcept override
  {
    const auto recv_tp = chrono::steady_clock::now();
    if (format == wscl::Data_format::utf8) {
      std::unique_ptr<jrpc::Response> res;
      try {
        res = jrpc::Response::make(data);
      } catch (const dmitigr::Exception& e) {
        std::clog << "invalid jrpc response: " << e.what();
        return;
      }
      DMITIGR_ASSERT(res);
      if (!res->id().IsInt())
        std::clog << "skipping jrpc response with non-integer ID\n";
      else if (auto* const e = dynamic_cast<jrpc::Error*>(res.get())) {
        std::clog << "jrpc error: " << e->code().value();
        const std::string_view what{e->what()};
        if (!what.empty())
          std::clog << " (" << what << ")";
        std::clog << "\n";
      } else if (auto* const r = dynamic_cast<jrpc::Result*>(res.get())) {
        measure_response_delay(r->id().GetInt(), recv_tp, format);
      }
    } else if (format == wscl::Data_format::binary) {
      const auto is_sensor_data = static_cast<std::uint8_t>(data[0]) == 1;
      DMITIGR_ASSERT(is_sensor_data);
      const auto req_id = static_cast<std::uint8_t>(data[1]);
      measure_response_delay(req_id, recv_tp, format);
      const auto record_count = dmitigr::net::conv<std::uint16_t>(data.data() + 2, 2);
      if (!record_count) {
        std::clog << "Request " << static_cast<int>(req_id) << ": "
                  << response_total_records_ << " records received\n";
        response_total_records_ = 0;
      } else
        response_total_records_ += record_count;
    }
  }

  void handle_error(const int code, const std::string_view message) noexcept override
  {
    DMITIGR_ASSERT(!is_open());
    std::clog << "Connection error: " << code << " (" << message << ")\n";
  }

  void handle_close(const int code, const std::string_view reason) noexcept override
  {
    DMITIGR_ASSERT(!is_open());
    std::clog << "Connection closed: " << code << " (" << reason << ")\n";
  }

  void measure_response_delay(const int req_id,
    const chrono::steady_clock::time_point recv_tp,
    const wscl::Data_format format)
  {
    if (0 < req_id && req_id < 256) {
      const auto sent_tp = timepoints_[req_id - 1];
      const auto dur = recv_tp - sent_tp;
      const auto [type, threshold] = [this, format]
      {
        return format == wscl::Data_format::utf8 ?
          std::make_pair("RPC", rpc_response_delay_threshold_) :
          std::make_pair("data", data_response_delay_threshold_);
      }();

      if (format == wscl::Data_format::utf8)
        max_rpc_response_delay_ = std::max(max_rpc_response_delay_,
          chrono::duration_cast<ms>(dur));
      else
        max_data_response_delay_ = std::max(max_data_response_delay_,
          chrono::duration_cast<ms>(dur));

      if (threshold > ms::zero() && dur >= threshold) {
        std::clog << "Attention! " << type << " response delay of request "
                  << req_id << " is "
                  << chrono::duration_cast<ms>(dur).count() << " ms\n";
        if (format == wscl::Data_format::utf8)
          ++bad_rpc_response_count_;
        else
          ++bad_data_response_count_;
      }
    } else
      std::clog << "skipping jrpc result with ID not in range [1, 255]\n";
  }
};

Connection make_connection(uwsc_loop* const loop)
{
  return Connection{chrono::milliseconds{100}, chrono::milliseconds{500},
    loop,
    wscl::Connection_options{}
    .set_host("rpi")
    .set_port(8888)
    .set_ping_interval(chrono::seconds{10})};
}

void run()
{
  uv::Loop loop;

  auto conn = make_connection(loop.native());

  uv::Timer pinger{loop};
  DMITIGR_ASSERT(!pinger.init_error());
  pinger.start([&conn, id = 1]() mutable
  {
    if (is_running) {
      // const auto end = chrono::system_clock::now() - chrono::seconds{10};
      // const auto beg = end - chrono::seconds{1};
      jrpc::Request req{id, "getData"};
      // req.set_parameter("begin", chrono::duration_cast<chrono::microseconds>(
      //     beg.time_since_epoch()).count());
      req.set_parameter("begin", -1000000);
      // req.set_parameter("end", chrono::duration_cast<chrono::microseconds>(
      //     end.time_since_epoch()).count());
      conn.send(req);
      id = id % 255 + 1; // [1, 255]
    }
  }, 1000, 1000);

  uv::Signal shutdowner{loop};
  DMITIGR_ASSERT(!shutdowner.init_error());
  shutdowner.start([&shutdowner, &pinger, &loop]
  {
    is_running = false;
    pinger.stop();
    loop.stop();
    shutdowner.stop();
  }, SIGINT);

  loop.run(UV_RUN_DEFAULT);

  std::cout << "Max RPC response delay = " << conn.max_rpc_response_delay().count() << '\n'
            << "Max data response delay = " << conn.max_data_response_delay().count() << '\n'
            << "Bad RPC response count = " << conn.bad_rpc_response_count() << '\n'
            << "Bad data response count = " << conn.bad_data_response_count() << '\n';
}

int main()
{
  is_running = true;
  run();
}
