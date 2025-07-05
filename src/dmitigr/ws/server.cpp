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
#include "../http/errc.hpp"
#include "connection.hpp"
#include "exceptions.hpp"
#include "http_io.hpp"
#include "http_request.hpp"
#include "server.hpp"
#include "server_options.hpp"
#include "util.hpp"
#include "uwebsockets.hpp"

#include <algorithm>
#include <limits>
#include <vector>
#include <utility>

// #define DMITIGR_WS_DEBUG

namespace dmitigr::ws {
namespace detail {

/// The abstract representation of Server.
class iServer {
public:
  using Options = Server_options;

  virtual ~iServer() = default;
  virtual const Options& options() const noexcept = 0;
  virtual void loop_submit(std::function<void()> callback) noexcept = 0;
  virtual void* loop() const noexcept = 0;
  virtual bool is_started() const noexcept = 0;
  virtual void start() = 0;
  virtual void stop() noexcept = 0;
  virtual void close_connections(int code, std::string reason) noexcept = 0;
  virtual void walk(std::function<void(Connection&)> callback) = 0;
  virtual std::size_t connection_count() const noexcept = 0;
};

/// The representation of Server.
template<bool IsSsl>
class Srv final : public iServer {
public:
  explicit Srv(Server* const server, void* const loop, Options options)
    : server_{server}
    , options_{std::move(options)}
  {
    if (loop)
      loop_ = uWS::Loop::get(loop);
    else
      throw Exception{"cannot use null event loop"};

    DMITIGR_ASSERT(loop && server_ && loop_);
  }

  const Options& options() const noexcept override
  {
    return options_;
  }

  void loop_submit(std::function<void()> callback) noexcept override
  {
    loop_->defer(std::move(callback));
  }

  void* loop() const noexcept override
  {
    return loop_;
  }

  bool is_started() const noexcept override
  {
    return listening_socket_;
  }

  void start() override
  {
    if (is_started())
      return;

    using App = uWS::CachingApp<IsSsl>;

    const auto ws_behavior = [this]
    {
      namespace chrono = std::chrono;
      typename App::template WebSocketBehavior<Ws_data> result;

      // These options are not yet exposed.
      result.compression = uWS::DISABLED;
      result.closeOnBackpressureLimit = false;
      result.resetIdleTimeoutOnSend = true;
      result.sendPingsAutomatically = true;
      result.maxLifetime = 0;

      using Timeout = decltype(result.idleTimeout);
      constexpr auto max_idle_timeout = std::numeric_limits<Timeout>::max();
      const auto idle_timeout = options().ws_idle_timeout()
        .value_or(chrono::seconds::zero());
      using Native_timeout = decltype(idle_timeout.count());
      static_assert(sizeof(Timeout) <= sizeof(Native_timeout));
      result.idleTimeout = std::min<Native_timeout>(max_idle_timeout,
        idle_timeout.count());

      using Payload = decltype(result.maxPayloadLength);
      constexpr auto max_payload_length = std::numeric_limits<Payload>::max();
      const auto max_incoming_payload_size = options().ws_max_incoming_payload_size()
        .value_or(std::numeric_limits<int>::max());
      using Native_payload = decltype(max_incoming_payload_size);
      static_assert(sizeof(Payload) <= sizeof(Native_payload));
      result.maxPayloadLength = max_incoming_payload_size ?
        std::min<Native_payload>(max_payload_length, max_incoming_payload_size) :
        max_payload_length;

      using Backpressure = decltype(result.maxBackpressure);
      constexpr auto max_backpressure = std::numeric_limits<Backpressure>::max();
      const auto backpressure_buffer_size = options().ws_backpressure_buffer_size()
        .value_or(std::numeric_limits<int>::max());
      using Native_backpressure = decltype(backpressure_buffer_size);
      static_assert(sizeof(Backpressure) <= sizeof(Native_backpressure));
      result.maxBackpressure = backpressure_buffer_size ?
        std::min<Native_backpressure>(max_backpressure, backpressure_buffer_size) :
        max_backpressure;

      result.upgrade = [this](uWS::HttpResponse<IsSsl>* const res,
        uWS::HttpRequest* const req, us_socket_context_t* const ctx)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .upgrade emitted" << std::endl;
#endif
        const iHttp_request request{req, res->getRemoteAddress(),
          local_address(IsSsl, reinterpret_cast<us_socket_t*>(res))};
        const auto io = std::make_shared<iHttp_io_templ<IsSsl>>(res, server_);
        Ws_data ws_data{server_->handle_handshake(request, io)};
        if (ws_data.conn) {
          if (!io->is_valid())
            throw Exception{"invalid instance of dmitigr::ws::Http_io"};
          const auto sec_ws_key = request.header("sec-websocket-key");
          const auto sec_ws_protocol = request.header("sec-websocket-protocol");
          const auto sec_ws_extensions = request.header("sec-websocket-extensions");
          if (!io->is_abort_handler_set()) {
            if (io->is_send_handler_set())
              throw Exception{"attempt to complete WebSocket handshake "
                "implicitly after setting a send handler"};
            res->upgrade(std::move(ws_data),
              sec_ws_key,
              sec_ws_protocol,
              sec_ws_extensions,
              ctx);
          } else {
            // Deferred handshake.
            io->ws_data_ = std::move(ws_data);
            io->sec_ws_key_ = sec_ws_key;
            io->sec_ws_protocol_ = sec_ws_protocol;
            io->sec_ws_extensions_ = sec_ws_extensions;
          }
          DMITIGR_ASSERT(io->ctx__() == ctx);
        } else if (io->is_valid() && !io->is_abort_handler_set()) {
          // Implicit rejection.
          io->send_status(http::Server_errc::internal_server_error);
          io->end();
        }
      };

      result.open = [this](auto* const ws)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .open emitted" << std::endl;
#endif
        auto* const ws_data = ws->getUserData();
        DMITIGR_ASSERT(ws_data);
        if (ws_data->conn) {
          ws_data->conn->rep_ = std::make_unique<Conn<IsSsl>>(ws, server_);
          connections_.emplace_back(static_cast<Conn<IsSsl>*>(
            ws_data->conn->rep_.get()));
          ws_data->conn->handle_open();
        } else
          ws->end(1011, "internal error");
      };

      result.message = [](auto* const ws, const std::string_view payload,
        const uWS::OpCode oc)
      {
        auto* const ws_data = ws->getUserData();
        DMITIGR_ASSERT(ws_data);
        DMITIGR_ASSERT(ws_data->conn);
        const auto format = (oc == uWS::OpCode::TEXT) ?
          Data_format::utf8 : Data_format::binary;
        ws_data->conn->handle_message(payload, format);
      };

      result.drain = [](auto* const ws)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .drain emitted. Backpressure buffer size is "
                  << ws->getBufferedAmount() << std::endl;
#endif
        auto* const ws_data = ws->getUserData();
        DMITIGR_ASSERT(ws_data);
        DMITIGR_ASSERT(ws_data->conn);
        ws_data->conn->handle_drain();
      };

      result.ping = [](auto* const /*ws*/, const std::string_view)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .ping emitted" << std::endl;
#endif
      };

      result.pong = [](auto* const /*ws*/, const std::string_view)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .pong emitted" << std::endl;
#endif
      };

      result.close = [this](auto* const ws, const int code,
        const std::string_view reason)
      {
#ifdef DMITIGR_WS_DEBUG
        std::clog << "dmitigr::ws: .close emitted" << std::endl;
#endif
        auto* const ws_data = ws->getUserData();
        DMITIGR_ASSERT(ws_data);
        /*
         * If connection is closed from .open or .upgrade,
         * then (ws_data->conn == nullptr) in such a case.
         */
        if (ws_data->conn) {
          ws_data->conn->handle_close(code, reason);
          if (!close_connections_called_) {
            auto* conn = static_cast<Conn<IsSsl>*>(ws_data->conn->rep_.get());
            const auto b = cbegin(connections_);
            const auto e = cend(connections_);
            const auto i = find_if(b, e, [conn](const auto p){return p == conn;});
            DMITIGR_ASSERT(i != e);
            connections_.erase(i);
          }
          {
            const std::lock_guard lg{ws_data->conn->mut_};
            ws_data->conn->rep_.reset();
          }
          ws_data->conn.reset();
        }
        DMITIGR_ASSERT(!ws_data->conn);
      };

      return result;
    }; // ws_behaviour

    const auto host = options().host().value_or("0.0.0.0");
    const auto port = options().port().value_or(80);

    /*
     * This is for Windows where std::filesystem::path::c_str()
     * returns const wchar_t*.
     */
    std::string ssl_private_key_file,
      ssl_certificate_file,
      ssl_dh_parameters_file,
      ssl_certificate_authority_file;
    App app = [this, &ssl_private_key_file, &ssl_certificate_file,
      &ssl_dh_parameters_file, &ssl_certificate_authority_file]
    {
      uWS::SocketContextOptions socket_options{};
      if (options().is_ssl_enabled().value_or(false)) {
        DMITIGR_ASSERT(IsSsl);
        if (const auto& value = options().ssl_private_key_file()) {
          ssl_private_key_file = value->string();
          socket_options.key_file_name = ssl_private_key_file.c_str();
        }
        if (const auto& value = options().ssl_certificate_file()) {
          ssl_certificate_file = value->string();
          socket_options.cert_file_name = ssl_certificate_file.c_str();
        }
        if (const auto& value = options().ssl_pem_file_password())
          socket_options.passphrase = value->c_str();
        if (const auto& value = options().ssl_dh_parameters_file()) {
          ssl_dh_parameters_file = value->string();
          socket_options.dh_params_file_name = ssl_dh_parameters_file.c_str();
        }
        if (const auto& value = options().ssl_certificate_authority_file()) {
          ssl_certificate_authority_file = value->string();
          socket_options.ca_file_name = ssl_certificate_authority_file.c_str();
        }
        socket_options.ssl_prefer_low_memory_usage = 0;
      }
      return App{socket_options};
    }();
    app.template ws<Ws_data>("/*", ws_behavior());
    if (options().is_http_enabled().value_or(false)) {
      app.any("/*", [this](auto* const res, auto* const req)
      {
        const iHttp_request request{req, res->getRemoteAddress(),
          local_address(IsSsl, reinterpret_cast<us_socket_t*>(res))};
        const auto io = std::make_shared<iHttp_io_templ<IsSsl>>(res, server_);
        server_->handle_request(request, io);
        if (io->is_valid() && !io->is_abort_handler_set())
          io->abort();
      });
    }
    app.listen(host, port, [this](auto* const listening_socket)
    {
      if (listening_socket)
        listening_socket_ = listening_socket;
      else
        throw Exception{"dmitigr::ws::Server is failed to bind the listening socket"};
    });
    app.run();
  }

  void stop() noexcept override
  {
    if (is_started()) {
      DMITIGR_ASSERT(listening_socket_);
      us_listen_socket_close(IsSsl, listening_socket_);
      listening_socket_ = nullptr;
    }
  }

  void close_connections(const int code, std::string reason) noexcept override
  {
    if (is_started()) {
      struct Guard {
        Guard(Srv& s) : s_{s} { s_.close_connections_called_ = true; }
        ~Guard() { s_.close_connections_called_ = false; }
      private:
        Srv& s_;
      } guard{*this};

      for (auto* const conn : connections_) {
        DMITIGR_ASSERT(conn);
        conn->close(code, reason);
      }
      connections_.clear();
    }
  }

  void walk(std::function<void(Connection&)> callback) override
  {
    for (auto* const conn : connections_) {
      DMITIGR_ASSERT(conn);
      callback(*conn->connection());
    }
  }

  std::size_t connection_count() const noexcept override
  {
    return connections_.size();
  }

private:
  Server* server_{};
  uWS::Loop* loop_{};
  Server_options options_;
  us_listen_socket_t* listening_socket_{};

  bool close_connections_called_{};
  std::vector<Conn<IsSsl>*> connections_;
};

using Non_ssl_server = Srv<false>;
#ifdef DMITIGR_LIBS_OPENSSL
using Ssl_server = Srv<true>;
#endif

} // namespace detail

DMITIGR_WS_INLINE Server::~Server() noexcept = default;

DMITIGR_WS_INLINE Server::Server(void* const loop, Options options)
{
#ifdef DMITIGR_LIBS_OPENSSL
  if (options.is_ssl_enabled().value_or(false))
    rep_ = std::make_unique<detail::Ssl_server>(this, loop,
      std::move(options));
  else
#endif
    rep_ = std::make_unique<detail::Non_ssl_server>(this, loop,
      std::move(options));

  DMITIGR_ASSERT(rep_);
}

DMITIGR_WS_INLINE auto Server::options() const noexcept -> const Options&
{
  const std::lock_guard lg{mut_};
  return rep_->options();
}

DMITIGR_WS_INLINE void Server::loop_submit(std::function<void()> callback) noexcept
{
  const std::lock_guard lg{mut_};
  rep_->loop_submit(std::move(callback));
}

DMITIGR_WS_INLINE const void* Server::loop() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->loop();
}

DMITIGR_WS_INLINE void* Server::loop() noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->loop();
}

DMITIGR_WS_INLINE bool Server::is_started() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->is_started();
}

DMITIGR_WS_INLINE void Server::start()
{
  rep_->start();
}

DMITIGR_WS_INLINE void Server::stop() noexcept
{
  const std::lock_guard lg{mut_};
  rep_->stop();
}

DMITIGR_WS_INLINE void
Server::close_connections(const int code, std::string reason) noexcept
{
  const std::lock_guard lg{mut_};
  rep_->close_connections(code, std::move(reason));
}

DMITIGR_WS_INLINE void Server::walk(std::function<void(Connection&)> callback)
{
  const std::lock_guard lg{mut_};
  rep_->walk(std::move(callback));
}

DMITIGR_WS_INLINE std::size_t Server::connection_count() const noexcept
{
  const std::lock_guard lg{mut_};
  return rep_->connection_count();
}

} // namespace dmitigr::ws

#ifdef DMITIGR_WS_DEBUG
#undef DMITIGR_WS_DEBUG
#endif
