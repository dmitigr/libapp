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

#include "../base/assert.hpp"
#include "../net/address.hpp"
#include "basics.hpp"
#include "connection.hpp"
#include "exceptions.hpp"
#include "util.hpp"
#include "uwebsockets.hpp"

namespace dmitigr::ws {
namespace detail {

/// The abstract representation of Connection.
class iConnection {
public:
  virtual ~iConnection() = default;
  virtual Connection* connection() const noexcept = 0;
  virtual const net::Ip_address& remote_ip_address() const noexcept = 0;
  virtual const net::Ip_address& local_ip_address() const noexcept = 0;
  virtual std::size_t buffered_amount() const noexcept = 0;
  virtual bool send(std::string_view payload, Data_format format) = 0;
  virtual void close(int code, std::string_view reason) noexcept = 0;
  virtual void abort() noexcept = 0;
  virtual bool is_closed() const noexcept = 0;
  virtual void loop_submit(std::function<void()> callback) noexcept = 0;
  virtual const Server* server() const noexcept = 0;
  virtual Server* server() noexcept = 0;
  virtual bool is_ssl() const noexcept = 0;
};

/// The representation of Connection.
template<bool IsSsl>
class Conn final : public iConnection {
public:
  using Underlying_type = uWS::WebSocket<IsSsl, true, Ws_data>;

  Conn(const Conn&) = delete;
  Conn& operator=(const Conn&) = delete;
  Conn(Conn&&) = delete;
  Conn& operator=(Conn&&) = delete;

  explicit Conn(Underlying_type* const ws, Server* const server)
    : ws_{ws}
    , server_{server}
  {
    DMITIGR_ASSERT(ws_ && server_);
    if (!is_closed()) {
      remote_ip_address_ = net::Ip_address::from_binary(ws_->getRemoteAddress());
      local_ip_address_ = net::Ip_address::from_binary(detail::local_address(
        is_ssl(), reinterpret_cast<us_socket_t*>(ws_)));
    } else { // just in case
      remote_ip_address_ = net::Ip_address::from_text("0.0.0.0");
      local_ip_address_ = net::Ip_address::from_text("0.0.0.0");
    }
  }

  Connection* connection() const noexcept override
  {
    auto* const ws_data = ws_->getUserData();
    DMITIGR_ASSERT(ws_data);
    return ws_data->conn.get();
  }

  const net::Ip_address& remote_ip_address() const noexcept override
  {
    return remote_ip_address_;
  }

  const net::Ip_address& local_ip_address() const noexcept override
  {
    return local_ip_address_;
  }

  std::size_t buffered_amount() const noexcept override
  {
    DMITIGR_ASSERT(!is_closed());
    return ws_->getBufferedAmount();
  }

  bool send(const std::string_view payload, const Data_format format) override
  {
    DMITIGR_ASSERT(!is_closed());
    return ws_->send(payload, (format == Data_format::utf8) ?
      uWS::OpCode::TEXT : uWS::OpCode::BINARY);
  }

  void close(const int code, const std::string_view reason) noexcept override
  {
    DMITIGR_ASSERT(!is_closed());
    ws_->end(code, reason);
    ws_ = nullptr;
  }

  void abort() noexcept override
  {
    DMITIGR_ASSERT(!is_closed());
    ws_->close();
    ws_ = nullptr;
  }

  bool is_closed() const noexcept override
  {
    DMITIGR_ASSERT(ws_);
    return us_socket_is_closed(is_ssl(), reinterpret_cast<us_socket_t*>(ws_));
  }

  void loop_submit(std::function<void()> callback) noexcept override
  {
    DMITIGR_ASSERT(!is_closed());
    auto* const uss = reinterpret_cast<us_socket_t*>(ws_);
    us_socket_context_t* const uss_ctx = us_socket_context(is_ssl(), uss);
    us_loop_t* const uss_loop = us_socket_context_loop(is_ssl(), uss_ctx);
    auto* const loop = reinterpret_cast<uWS::Loop*>(uss_loop);
    loop->defer(std::move(callback));
  }

  const Server* server() const noexcept override
  {
    return server_;
  }

  Server* server() noexcept override
  {
    return const_cast<Server*>(static_cast<const Conn*>(this)->server());
  }

  bool is_ssl() const noexcept override
  {
    return IsSsl;
  }

private:
  Underlying_type* ws_{};
  Server* server_{};
  net::Ip_address remote_ip_address_;
  net::Ip_address local_ip_address_;
};

} // namespace detail

DMITIGR_WS_INLINE Connection::~Connection() noexcept = default;
DMITIGR_WS_INLINE Connection::Connection() noexcept = default;

DMITIGR_WS_INLINE bool Connection::loop_submit(std::function<void()> callback) noexcept
{
  const std::lock_guard lg{mut_};
  if (rep_ && !rep_->is_closed()) {
    rep_->loop_submit(std::move(callback));
    return true;
  } else
    return false;
}

DMITIGR_WS_INLINE const Server* Connection::server() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_ ? rep_->server() : nullptr;
}

DMITIGR_WS_INLINE Server* Connection::server() noexcept
{
  return const_cast<Server*>(static_cast<const Connection*>(this)->server());
}

bool Connection::is_connected_nts() const noexcept
{
  return rep_ && !rep_->is_closed();
}

DMITIGR_WS_INLINE bool Connection::is_connected() const noexcept
{
  const std::lock_guard lg{mut_};
  return is_connected_nts();
}

DMITIGR_WS_INLINE const net::Ip_address& Connection::remote_ip_address() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->remote_ip_address();
}

DMITIGR_WS_INLINE const net::Ip_address& Connection::local_ip_address() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->local_ip_address();
}

DMITIGR_WS_INLINE std::size_t Connection::buffered_amount() const noexcept
{
  return is_connected_nts() ? rep_->buffered_amount() : 0;
}

DMITIGR_WS_INLINE bool Connection::send(const std::string_view payload,
  const Data_format format)
{
  if (!is_connected_nts())
    throw Exception{"cannot send data via invalid WebSocket connection"};

  return rep_->send(payload, format);
}

DMITIGR_WS_INLINE bool Connection::send_utf8(const std::string_view payload)
{
  return send(payload, Data_format::utf8);
}

DMITIGR_WS_INLINE bool Connection::send_binary(const std::string_view payload)
{
  return send(payload, Data_format::binary);
}

DMITIGR_WS_INLINE void Connection::close(const int code,
  const std::string_view reason) noexcept
{
  if (is_connected_nts())
    rep_->close(code, reason);
  DMITIGR_ASSERT(!is_connected_nts());
}

DMITIGR_WS_INLINE void Connection::abort() noexcept
{
  if (is_connected_nts())
    rep_->abort();
  DMITIGR_ASSERT(!is_connected_nts());
}

} // namespace dmitigr::ws
