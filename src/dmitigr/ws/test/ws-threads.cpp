// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin
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

#include "../../base/rnd.hpp"
#include "../../uv/uv.hpp"
#include "../../ws/ws.hpp"

#include <chrono>
#include <sstream>
#include <thread>
#include <vector>

namespace rnd = dmitigr::rnd;
namespace uv = dmitigr::uv;
namespace ws = dmitigr::ws;

namespace {

class Connection : public ws::Connection {
  /**
   * @brief Attempts to reply to the message from several threads.
   *
   * @details Emulates a situation when some random thread closes connection.
   */
  void handle_message(const std::string_view payload,
    const ws::Data_format format) noexcept override
  {
    std::clog << "The received payload \"" << payload << "\" is handled" << std::endl;
    std::vector<std::thread> workers{16};
    const auto closer_index = rnd::week_integer<std::size_t>(0, workers.size());
    for (std::size_t i = 0; i < workers.size(); ++i) {
      workers[i] = std::thread{[ws = shared_from_this(), format,
          is_closer = (i == closer_index)]
      {
        if (!is_closer) {
          ws->loop_submit([ws, format, tid = std::this_thread::get_id()]
          {
            if (ws->is_connected()) {
              std::ostringstream s;
              s << "Hello from thread " << tid;
              ws->send(s.str(), format);
            } else
              std::clog << "ignoring the received message from thread "
                        << tid << " (connection closed)" << std::endl;
          });
        } else
          ws->loop_submit([ws]{ws->close(0, "closed (expected)");});
      }};
    }
    for (auto& w : workers)
      w.join();
  }

  void handle_open() noexcept override
  {
    auto ip = remote_ip_address().to_string();
    std::clog << "The connection to " << ip << " is open" << std::endl;
  }

  void handle_close(const int /*code*/,
    const std::string_view /*reason*/) noexcept override
  {
    auto ip = remote_ip_address().to_string();
    std::clog << "The connection to " << ip << " is about to close" << std::endl;
  }

  void handle_drain() noexcept override {}
};

class Server : public ws::Server {
  using ws::Server::Server;

  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {
    std::clog << "The connection is about to be opened" << std::endl;
    const bool is_should_be_created = rnd::week_integer(0, 1);
    return is_should_be_created ?
      std::shared_ptr<Connection>{new (std::nothrow) Connection} : nullptr;
  }
  void handle_request(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {}
};

} // namespace

int main()
{
  using namespace std::chrono;

  rnd::seed_by_now();

  try {
    constexpr auto listening_duration = seconds{15};
    Server server{uv_default_loop(), []
    {
      ws::Server_options so;
      so.set_host("127.0.0.1")
        .set_port(9001)
        .set_ws_idle_timeout(seconds{4000})
        .set_ws_max_incoming_payload_size(16 * 1024);
      std::clog << "The WebSocket idle timeout is set to " <<
        std::chrono::duration_cast<seconds>(
          so.ws_idle_timeout().value()).count()
            << " seconds " << std::endl
            << "The WebSocket max incoming payload size is set to "
            << so.ws_max_incoming_payload_size().value_or(0) << std::endl;
      return so;
    }()};
    std::thread server_thread{[&server, listening_duration]
    {
      std::clog << "Starting WebSocket server. Listening socket will be closed in "
                << listening_duration.count() << " seconds." << std::endl;
      server.start();
    }};
    std::thread finalizer_thread{[&server, listening_duration]
    {
      std::this_thread::sleep_for(listening_duration);
      server.stop();
      std::clog << "The WebSocket server is closed." << std::endl;
    }};
    finalizer_thread.join();
    server_thread.join();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
