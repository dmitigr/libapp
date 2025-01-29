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
#include "../../http/client.hpp"

int main(const int argc, char* const argv[])
{
  try {
    namespace chrono = std::chrono;
    namespace http = dmitigr::http;
    namespace net = dmitigr::net;

    if (argc < 3) {
      std::cerr << "usage: http-client address port" << std::endl;
      return 1;
    }
    const std::string address{argv[1]};
    const int port = std::stoi(std::string{argv[2]});

    auto conn = http::Client_connection::make({address, port});
    DMITIGR_ASSERT(!conn->is_server());
    conn->connect();
    conn->send_start(http::Method::get, "/");
    conn->send_header("User-Agent", "dmitigr::http");
    conn->send_last_header("Accept", "*/*");
    conn->receive_head();
    if (!conn->is_head_received())
      throw std::runtime_error{"cannot receive HTTP head"};

    std::string content;
    if (conn->content_length() >= 1048576)
      throw std::runtime_error{"HTTP payload too large"};
    else
      content = conn->receive_content_to_string();

    std::string response{"Start line:\n"};
    response.append("version = ").append(conn->version()).append("\n");
    response.append("status code = ").append(conn->status_code()).append("\n");
    response.append("status phrase = ").append(conn->status_phrase()).append("\n");
    response.append("Headers:\n");
    for (const auto& rh : conn->headers()) {
      response += rh.name();
      response += "<-->";
      response += rh.value();
      response += "\n";
    }
    if (!content.empty()) {
      response.append("Content:\n");
      response.append(content);
    }
    std::cout << response;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
