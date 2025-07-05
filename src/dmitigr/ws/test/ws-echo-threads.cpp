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

#include "../../uv/uv.hpp"
#include "../../ws/ws.hpp"

#include <chrono>
#include <sstream>
#include <thread>

namespace ws = dmitigr::ws;

// The implementation of the thread per receive approach.
// (Using a thread pool approach instead is trivial.)
class Connection final : public ws::Connection {
  void handle_message(const std::string_view payload,
    const ws::Data_format format) noexcept override
  {
    std::thread{[ws = shared_from_this(), payload, format]
    {
      // Simulate a work.
      std::this_thread::sleep_for(std::chrono::seconds(3));

      // Respond by using non-blocking IO.
      ws->loop_submit([ws, payload, format, tid = std::this_thread::get_id()]
      {
        if (ws->is_connected()) {
          std::ostringstream s;
          s << "Echo from thread " << tid << ": " << payload;
          ws->send(s.str(), format);
        }
      });
    }}.detach();
  }

  void handle_open() noexcept override {}
  void handle_close(int, std::string_view) noexcept override {}
  void handle_drain() noexcept override {}
};

class Server final : public ws::Server {
  using ws::Server::Server;
  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {
    return std::shared_ptr<Connection>{new (std::nothrow) Connection};
  }
  void handle_request(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {}
};

int main()
{
  Server{uv_default_loop(), ws::Server_options{}
    .set_port(9001)
    .set_ws_idle_timeout(std::chrono::seconds{10})
    .set_ws_max_incoming_payload_size(16 * 1024)}.start();
}
