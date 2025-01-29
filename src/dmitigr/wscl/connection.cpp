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

#include "connection.hpp"
#include "exceptions.hpp"
#include "../base/assert.hpp"

#include <cstring>

namespace std {

/// The default deleter for uwsc_client.
template<> struct default_delete<uwsc_client> final {
  void operator()(uwsc_client* const ptr) const
  {
    ptr->free(ptr);
  }
};

} // namespace std

namespace dmitigr::wscl {

struct Connection::Rep final {
  ~Rep()
  {
    close(UWSC_CLOSE_STATUS_NORMAL);
  }

  Rep(Connection& self, uwsc_loop* const loop, Options options)
    : options_{std::move(options)}
  {
    if (!loop)
      throw Exception{"cannot attach WebSocket client connection to invalid loop"};

#ifdef DMITIGR_LIBS_OPENSSL
    if (options_.is_ssl_enabled() && *options_.is_ssl_enabled()) {
      const auto ca_crt = options_.ssl_certificate_authority_file();
      if (!ca_crt)
        throw Exception{"cannot create WebSocket client SSL connection: "
          "certificate authority file not specified"};
      const auto crt = options_.ssl_certificate_file();
      if (!crt)
        throw Exception{"cannot create WebSocket client SSL connection: "
          "certificate file not specified"};
      const auto key = options_.ssl_private_key_file();
      if (!key)
        throw Exception{"cannot create WebSocket client SSL connection: "
          "private key file not specified"};
      uwsc_load_ca_crt_file(ca_crt->string().c_str());
      uwsc_load_crt_file(crt->string().c_str());
      uwsc_load_key_file(key->string().c_str());
    }
#endif

    const auto err = uwsc_init(&client_, loop, options_.url().c_str(),
      options_.ping_interval().value_or(std::chrono::seconds::zero()).count(),
      options_.extra_headers_string().c_str());
    if (err)
      throw Exception{"cannot initialize WebSocket client connection"};

    client_.ext = &self;
    client_.onopen = &Connection::handle_open__;
    client_.onmessage = &Connection::handle_message__;
    client_.onerror = &Connection::handle_error__;
    client_.onclose = &Connection::handle_close__;
  }

  void set_ping_interval(const std::chrono::seconds interval)
  {
    if (!is_open_)
      throw Exception{"cannot set ping interval of invalid WebSocket client "
        "connection"};

    options_.set_ping_interval(interval);
    client_.ping_interval = interval.count();
  }

  void send(const std::string_view data, const Data_format format)
  {
    if (!is_open_)
      throw Exception{"cannot send via invalid WebSocket client connection"};

    client_.send(&client_, data.data(), data.size(),
      format == Data_format::binary ? UWSC_OP_BINARY : UWSC_OP_TEXT);
  }

  void ping()
  {
    if (!is_open_)
      throw Exception{"cannot ping via invalid WebSocket client connection"};

    client_.ping(&client_);
  }

  void close(const int code, const std::string& reason = {}) noexcept
  {
    if (is_open_)
      client_.send_close(&client_, code, reason.c_str());
  }

  bool is_open_{};
  Options options_;
  uwsc_client client_;
};

DMITIGR_WSCL_INLINE Connection::~Connection() = default;

DMITIGR_WSCL_INLINE Connection::Connection(uwsc_loop* const loop, Options options)
{
  reset(loop, std::move(options));
}

DMITIGR_WSCL_INLINE void Connection::reset(uwsc_loop* const loop, Options options)
{
  rep_ = std::make_unique<Rep>(*this, loop, std::move(options));
}

DMITIGR_WSCL_INLINE const uwsc_loop* Connection::loop() const noexcept
{
  return rep_->client_.loop;
}

DMITIGR_WSCL_INLINE uwsc_loop* Connection::loop() noexcept
{
  return rep_->client_.loop;
}

DMITIGR_WSCL_INLINE bool Connection::is_open() const noexcept
{
  return rep_->is_open_;
}

DMITIGR_WSCL_INLINE auto Connection::options() const noexcept -> const Options&
{
  return rep_->options_;
}

DMITIGR_WSCL_INLINE void
Connection::set_ping_interval(const std::chrono::seconds interval)
{
  rep_->set_ping_interval(interval);
}

DMITIGR_WSCL_INLINE void Connection::send(const std::string_view data,
  const Data_format format)
{
  rep_->send(data, format);
}

DMITIGR_WSCL_INLINE void Connection::send_utf8(const std::string_view data)
{
  send(data, Data_format::utf8);
}

DMITIGR_WSCL_INLINE void Connection::send_binary(const std::string_view data)
{
  send(data, Data_format::binary);
}

DMITIGR_WSCL_INLINE void Connection::ping()
{
  rep_->ping();
}

DMITIGR_WSCL_INLINE void Connection::close(const int code,
  const std::string& reason) noexcept
{
  rep_->close(code, reason);
}

DMITIGR_WSCL_INLINE void Connection::handle_open__(uwsc_client* const cl) noexcept
{
  auto* const s = self(cl);
  s->rep_->is_open_ = true;
  s->handle_open();
}

DMITIGR_WSCL_INLINE void Connection::handle_message__(uwsc_client* const cl,
  void* const data, const std::size_t size, const bool binary) noexcept
{
  self(cl)->handle_message({static_cast<char*>(data), size},
    binary ? Data_format::binary : Data_format::utf8);
}

DMITIGR_WSCL_INLINE void Connection::handle_error__(uwsc_client* const cl,
  const int code, const char* const message) noexcept
{
  auto* const s = self(cl);
  s->rep_->is_open_ = false;
  s->handle_error(code, {message, std::strlen(message)});
}

DMITIGR_WSCL_INLINE void Connection::handle_close__(uwsc_client* const cl,
  const int code, const char* const reason) noexcept
{
  auto* const s = self(cl);
  s->rep_->is_open_ = false;
  s->handle_close(code, {reason, std::strlen(reason)});
}

DMITIGR_WSCL_INLINE Connection* Connection::self(uwsc_client* const cl) noexcept
{
  DMITIGR_ASSERT(cl);
  auto* const self = static_cast<Connection*>(cl->ext);
  DMITIGR_ASSERT(self);
  return self;
}

} // namespace dmitigr::wscl
