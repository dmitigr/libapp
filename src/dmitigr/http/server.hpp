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

#ifndef DMITIGR_HTTP_SERVER_HPP
#define DMITIGR_HTTP_SERVER_HPP

#include "../base/assert.hpp"
#include "../net/listener.hpp"
#include "connection.hpp"
#include "errc.hpp"
#include "types_fwd.hpp"

#include <string>
#include <type_traits>

namespace dmitigr::http {

/// A HTTP server connection.
class Server_connection final : public Connection {
public:
  /// @see Connection::is_server().
  bool is_server() const override
  {
    return true;
  }

  /// Sends start line.
  void send_start(const Server_errc code, const bool skip_headers = false)
  {
    const char* const literal = to_literal(code);
    DMITIGR_ASSERT(literal);
    std::string line;
    line.reserve(9 + 3 + 1 + std::strlen(literal) + 2);
    line.append("HTTP/1.0 ").append(std::to_string(static_cast<int>(code)))
      .append(" ").append(literal).append("\r\n");
    if (skip_headers) {
      line.append("\r\n");
      is_headers_sent_ = true;
    }
    send_start__(line);
  }

  /// Alternative of `send_start(name, value, true)`.
  template<class StatusCode>
  void send_start_skip_headers(const StatusCode code)
  {
    send_start(code, true);
  }

  /// @returns The method extracted from start line.
  std::string_view method() const
  {
    const char* const offset = head_.data();
    return {offset, method_size_};
  }

  /// @returns The path extracted from start line.
  std::string_view path() const
  {
    const char* const offset = head_.data() + method_size_ + 1;
    return {offset, path_size_};
  }

  /// @returns The HTTP version extracted from start line.
  std::string_view version() const override
  {
    const char* const offset = head_.data() + method_size_ + 1 + path_size_ + 1;
    return {offset, version_size_};
  }

private:
  friend Listener;
  using Connection::Connection;
};

/// HTTP Listener options.
class Listener_options final {
public:
#ifdef _WIN32
  /// @see net::Listener_options.
  explicit Listener_options(std::string pipe_name)
    : listener_options_{std::move(pipe_name)}
  {}
#else
  /// @see net::Listener_options.
  Listener_options(std::filesystem::path path, int backlog)
    : listener_options_{std::move(path), backlog}
  {}
#endif
  /// @see net::Listener_options.
  Listener_options(std::string address, int port, int backlog)
    : listener_options_{std::move(address), port, backlog}
  {}

  /// @returns A new instance of the Listener initialized with this instance.
  Listener make_listener() const;

  /// @see net::Listener_options.
  const net::Endpoint& endpoint() const
  {
    return listener_options_.endpoint();
  }

  /// @see net::Listener_options.
  std::optional<int> backlog() const
  {
    return listener_options_.backlog();
  }

private:
  friend Listener;
  net::Listener_options listener_options_;
};

/// HTTP listener.
class Listener final {
public:
  /// An alias of Listener_options.
  using Options = Listener_options;

  /// The constructor.
  explicit Listener(Options options)
    : listener_{net::Listener::make(options.listener_options_)}
    , options_{std::move(options)}
  {}

  /// @returns The listener options.
  const Options& options() const
  {
    return options_;
  }

  /// @see net::Listener::is_listening().
  bool is_listening() const
  {
    return listener_->is_listening();
  }

  /// @see net::Listener::listen().
  void listen()
  {
    listener_->listen();
  }

  /// @see net::Listener::wait().
  bool wait(const std::chrono::milliseconds timeout
    = std::chrono::milliseconds{-1})
  {
    return listener_->wait(timeout);
  }

  /// Accepts new incoming connection..
  std::unique_ptr<Server_connection> accept();

  /// @see net::Listener::accept().
  void close()
  {
    listener_->close();
  }

private:
  std::unique_ptr<net::Listener> listener_;
  Listener_options options_;
};

inline Listener Listener_options::make_listener() const
{
  return Listener{*this};
}

std::unique_ptr<Server_connection> Listener::accept()
{
  auto io = listener_->accept();
  return std::unique_ptr<Server_connection>{new Server_connection{std::move(io)}};
}

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_SERVER_HPP
