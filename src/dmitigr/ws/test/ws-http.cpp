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

#include <cstdint>
#include <iostream>

namespace ws = dmitigr::ws;

class Connection final : public ws::Connection {
  void handle_message(const std::string_view payload,
    const ws::Data_format format) noexcept override
  {
    send(payload, format); // respond by using non-blocking IO
  }

  void handle_open() noexcept override {}
  void handle_close(int, std::string_view) noexcept override {};
  void handle_drain() noexcept override {};
};

class Server final : public ws::Server {
  using ws::Server::Server;
  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {
    return std::shared_ptr<Connection>{new (std::nothrow) Connection};
  }

  void handle_request(const ws::Http_request&,
    std::shared_ptr<ws::Http_io> io) noexcept override
  {
    io->set_receive_handler([
        total_size = std::uintmax_t{}](auto data, bool is_last) mutable
    {
      total_size += data.size();
      std::cout << "Received portion of data, size =  " << data.size() << std::endl;
      if (is_last)
        std::cout << "Received the last portion of data! Total data size = "
                  << total_size << std::endl;
    });

    io->set_abort_handler([]
    {
      std::cout << "Invoked abort handler" << std::endl;
    });

    io->send_header("Content-Type", "text/plain");
    io->send_header("Content-Disposition", "filename=ws-http-test-data.txt");
    const auto content = std::make_shared<std::string>(32'000'000, 'a');
    io->set_send_handler([io, content](const std::uintmax_t pos) -> bool
    {
      std::cout << "Ready to send handler invoked. Current content position = "
                << pos << std::endl;
      std::string_view dv{content->data() + pos, content->size() - pos};
      const auto [ok, done] = io->send_content(dv, content->size());
      (void)done;
      return ok;
    });
    io->send_content(*content);
  }
};

int main()
{
  Server{uv_default_loop(), ws::Server_options{}
    .set_port(9001)
    .set_http_enabled(true)
    .set_ws_idle_timeout(std::chrono::seconds{120})
    .set_ws_max_incoming_payload_size(16 * 1024)}.start();
}
