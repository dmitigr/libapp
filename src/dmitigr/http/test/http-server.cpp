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
#include "../../http/server.hpp"

int main()
{
  namespace chrono = std::chrono;
  namespace http = dmitigr::http;
  namespace net = dmitigr::net;

  const http::Listener_options lo{"127.0.0.1", 8888, 128};
  auto l = lo.make_listener();
  l.listen();
  while (true) {
    try {
      auto conn = l.accept();
      const auto nh = conn->native_handle();
      constexpr chrono::seconds head_timeout{3};
      net::set_timeout(static_cast<net::Socket_native>(nh), head_timeout, head_timeout);
      conn->receive_head();
      if (!conn->is_head_received()) {
        conn->send_start(http::Server_errc::bad_request);
        continue;
      } else if (conn->content_length() >= 1048576) {
        conn->send_start(http::Server_errc::payload_too_large);
        continue;
      }

      constexpr chrono::seconds content_timeout{10};
      net::set_timeout(static_cast<net::Socket_native>(nh), content_timeout, content_timeout);
      const auto content = conn->receive_content_to_string();

      std::string response{"Start line:\n"};
      response.append("method = ").append(conn->method()).append("\n");
      response.append("path = ").append(conn->path()).append("\n");
      response.append("version = ").append(conn->version()).append("\n\n");
      if (!conn->headers().empty()) {
        response.append("Headers:\n");
        for (const auto& rh : conn->headers()) {
          response += rh.name();
          response += "<-->";
          response += rh.value();
          response += "\n";
        }
      }
      if (!content.empty()) {
        response.append("Content:\n");
        response.append(content);
      }

      conn->send_start(http::Server_errc::ok);
      conn->send_header("Server", "dmitigr");
      conn->send_header("Content-Type", "text/plain");
      //conn->send_header("Content-Disposition", "attachment; filename=a.txt");
      conn->send_last_header("Content-Length", std::to_string(response.size()));
      DMITIGR_ASSERT(conn->unsent_content_length() == response.size());
      conn->send_content(response);
      DMITIGR_ASSERT(!conn->unsent_content_length());
    } catch (const std::system_error& e) {
      if (e.code() != std::errc::resource_unavailable_try_again &&
        e.code()   != std::errc::broken_pipe &&
        e.code()   != std::errc::connection_reset)
        std::cerr << "System error: " << e.code() << " " << strerror(e.code().value()) << std::endl;
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    } catch (...) {
      std::cerr << "unknown error" << std::endl;
    }
  }
}
