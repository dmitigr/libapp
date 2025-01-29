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

#ifndef DMITIGR_HTTP_CLIENT_HPP
#define DMITIGR_HTTP_CLIENT_HPP

#include "../base/assert.hpp"
#include "../net/client.hpp"
#include "connection.hpp"
#include "exceptions.hpp"
#include "types_fwd.hpp"

namespace dmitigr::http {

/// A HTTP client connection.
class Client_connection final : public Connection {
public:
  /// The constructor.
  Client_connection(net::Client_options options)
    : options_{std::move(options)}
  {}

  /// @returns Newly created instance.
  static std::unique_ptr<Client_connection> make(net::Client_options options)
  {
    return std::make_unique<Client_connection>(std::move(options));
  }

  /// @returns The options.
  const net::Client_options& options() const
  {
    return options_;
  }

  /// @see Connection::is_server().
  bool is_server() const override
  {
    return false;
  }

  /// Connects to remote host.
  void connect()
  {
    init(net::make_tcp_connection(options_));
  }

  /**
   * @par Requires
   * `!is_head_received()`.
   */
  void send_start(const Method method, const std::string_view path,
    const bool skip_headers = false)
  {
    if (is_head_received())
      throw Exception{"cannot send HTTP start line because head received"};

    const auto m{to_string_view(method)};
    DMITIGR_ASSERT(!m.empty());
    std::string line;
    line.reserve(7 + 1 + path.size() + 11);
    line.append(m).append(" ").append(path).append(" HTTP/1.0\r\n");
    if (skip_headers) {
      line.append("\r\n");
      is_headers_sent_ = true;
    }
    send_start__(line);
  }

  /// Alternative of `send_start(name, value, true)`.
  void send_start_skip_headers(const Method method, const std::string_view path)
  {
    send_start(method, path, true);
  }

  /// @returns The HTTP version extracted from start line.
  std::string_view version() const override
  {
    const char* const offset = head_.data();
    return {offset, version_size_};
  }

  /// @returns The status code extracted from start line.
  std::string_view status_code() const
  {
    const char* const offset = head_.data() + version_size_ + 1;
    return {offset, code_size_};
  }

  /// @returns The status phrase extracted from start line.
  std::string_view status_phrase() const
  {
    const char* const offset = head_.data() + version_size_ + 1 + code_size_ + 1;
    return {offset, phrase_size_};
  }

private:
  net::Client_options options_;
};

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_CLIENT_HPP
