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

#include "../../uv/uv.hpp"
#include "../../web/wsjrpc.hpp"

namespace uv = dmitigr::uv;
namespace web = dmitigr::web;
namespace ws = dmitigr::ws;

class Connection final : public web::Ws_jrpc_connection {
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
  using namespace std::chrono;
  uv::Loop loop;
  Server server{loop.native(), ws::Server_options{}
    .set_port(9001)
    .set_ws_idle_timeout(seconds{8})
    .set_ws_max_incoming_payload_size(16 * 1024)};

  uv::Signal watcher{loop};
  watcher.start([&server, &watcher]
  {
    server.close_connections(1000, "server graceful shutdown");
    server.stop();
    watcher.stop();
    std::clog << "Graceful shutdown." << std::endl;
  }, SIGINT);

  server.start();
}
