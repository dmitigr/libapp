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

#include "../../base/algorithm.hpp"

#include "../io.hpp"

#include <string>
#include <vector>

namespace {

class Proxy_connection;

struct Proxy_options final {
  boost::asio::ip::tcp::endpoint listen_endpoint;
  boost::asio::ip::tcp::endpoint server_endpoint;
};

class Proxy final : public dmitigr::io::Acceptor {
public:
  using Options = Proxy_options;

  Proxy(Must_be_shared_ptr, boost::asio::io_context& loop, Options options)
    : Acceptor{Must_be_shared_ptr{},
      boost::asio::ip::tcp::acceptor{loop, options.listen_endpoint}}
    , loop_{loop}
    , options_{std::move(options)}
  {
    acceptor_.listen();
  }

  /// @see Proxy().
  template<typename ... Types>
  static auto make(Types&& ... args)
  {
    return std::make_shared<Proxy>(Must_be_shared_ptr{},
      std::forward<Types>(args)...);
  }

private:
  friend Proxy_connection;

  boost::asio::io_context& loop_;
  Options options_;
  std::vector<std::shared_ptr<Proxy_connection>> connections_;

private:
  void handle_accept(dmitigr::io::Connection) noexcept override;
  void finish(const std::error_code&) noexcept override;
};

class Proxy_connection final : public dmitigr::io::Proxy_connection {
public:
  using Super = dmitigr::io::Proxy_connection;

  Proxy_connection(Must_be_shared_ptr,
    dmitigr::io::Connection connection, std::shared_ptr<Proxy> proxy)
    : Super{Must_be_shared_ptr{}, std::move(connection)}
    , proxy_{std::move(proxy)}
  {
    if (!proxy_)
      throw std::invalid_argument{"cannot create instance of Proxy_connection"};
  }

  /// @see dmitigr::io::Proxy_connection::Proxy_connection().
  template<typename ... Types>
  static auto make(Types&& ... args)
  {
    return std::make_shared<Proxy_connection>(Must_be_shared_ptr{},
      std::forward<Types>(args)...);
  }

private:
  std::shared_ptr<Proxy> proxy_;
  std::string buf_;

  void handle_read_ready() override
  {
    buf_.resize(connection().socket.available());
    const auto buf = boost::asio::buffer(buf_);
    read(connection().socket, buf);
    async_write_to_linked(buf);
  }

  void handle_wrote_to_linked(const std::size_t /*byte_count*/) override
  {}

  void finish(const std::error_code& /*error*/) noexcept override
  {
    const auto self = shared_from_this();
    dmitigr::erase(proxy_->connections_, self);
    self->connection().socket.close();
    const auto linked_w = self->linked_connection();
    if (const auto linked = linked_w.lock()) {
      dmitigr::erase(proxy_->connections_, linked);
      linked->connection().socket.close();
    }
  }
};

// -----------------------------------------------------------------------------
// Proxy implementation
// -----------------------------------------------------------------------------

inline void Proxy::handle_accept(dmitigr::io::Connection conn_cl) noexcept
{
  const auto self = std::static_pointer_cast<Proxy>(shared_from_this());

  // Create Proxy_connection (Proxy - Server).
  const auto conn_server = Proxy_connection::make(
    dmitigr::io::Connection{boost::asio::ip::tcp::socket{self->loop_}}, self);

  // Connect to server.
  conn_server->connection().socket.async_connect(options_.server_endpoint,
    [ // Create Proxy_connection (Client - Proxy).
      conn_client = Proxy_connection::make(std::move(conn_cl), self),
      conn_server, self]
    (const std::error_code& error)
    {
      if (error) {
        self->finish(error);
        return;
      }

      try {
        // Link proxy connections.
        conn_client->link(conn_server);

        // Store proxy connections.
        self->connections_.push_back(conn_client);
        self->connections_.push_back(conn_server);

        // Initiate waiting for input.
        conn_client->async_wait_read_ready();
        conn_server->async_wait_read_ready();

        // Accept again.
        self->async_accept();
      } catch (...) {
        self->finish(make_error_code(std::errc::operation_canceled));
      }
    });
}

inline void Proxy::finish(const std::error_code& /*err*/) noexcept
{
  async_accept();
}

} // namespace

int main()
{
  boost::asio::io_context loop;
  Proxy_options po;
  po.listen_endpoint = {boost::asio::ip::make_address("127.0.0.1"), 2345};
  po.server_endpoint = {boost::asio::ip::make_address("192.168.56.2"), 5432};
  auto proxy = Proxy::make(loop, std::move(po));
  proxy->async_accept();
  loop.run();
}
