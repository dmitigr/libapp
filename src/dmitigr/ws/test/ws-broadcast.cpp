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

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>

namespace uv = dmitigr::uv;
namespace ws = dmitigr::ws;

std::atomic_bool is_running;

class Connection;

class Server final : public ws::Server {
  using ws::Server::Server;
  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override;
  void handle_request(const ws::Http_request&,
    std::shared_ptr<ws::Http_io>) noexcept override
  {}
};

class Connection final : public ws::Connection {
private:
  void handle_message(const std::string_view payload,
    const ws::Data_format format) noexcept override
  {
    server()->walk([=](auto& conn)
    {
      conn.send(payload, format); // respond by using non-blocking IO
    });
  }

  void handle_open() noexcept override {}
  void handle_close(int, std::string_view) noexcept override {}
  void handle_drain() noexcept override {}
};

std::shared_ptr<ws::Connection> Server::handle_handshake(const ws::Http_request&,
  std::shared_ptr<ws::Http_io>) noexcept
{
  return std::shared_ptr<Connection>{new (std::nothrow) Connection};
}

int main()
{
  using namespace std::chrono;
  uv::Loop loop;
  DMITIGR_ASSERT(!loop.init_error());
  Server server{loop.native(), ws::Server_options{}
    .set_port(9001)
    .set_ws_idle_timeout(seconds{8})
    .set_ws_max_incoming_payload_size(16 * 1024)};

  uv::Signal closer{loop};
  DMITIGR_ASSERT(!closer.init_error());
  closer.start([&server, &closer]
  {
    server.close_connections(1000, "server graceful shutdown");
    server.stop();
    closer.stop();
    is_running = false;
    std::clog << "Graceful shutdown." << std::endl;
  }, SIGINT);

  uv::Timer pinger{loop};
  DMITIGR_ASSERT(!pinger.init_error());
  pinger.start([&server, &pinger]
  {
    if (is_running) {
      std::cout << "sending \"ping\"...";
      server.walk([](auto& conn)
      {
        conn.send_utf8("ping");
      });
      std::cout << "done\n";
    } else
      pinger.stop();
  }, 1000, 1000);

  is_running = true;
  server.stop();
}
