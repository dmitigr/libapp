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

#include "../base/assert.hpp"
#include "../net/util.hpp"
#include "basics.hpp"
#include "exceptions.hpp"
#include "server_options.hpp"

#include <limits>

namespace dmitigr::ws {
namespace detail {

inline namespace validators {

template<typename T>
inline bool is_valid_port(const T value) noexcept
{
  return 0 < value && value < 65536;
}

inline bool is_ip_address(const std::string& value) noexcept
{
  return net::Ip_address::is_valid(value);
}

inline bool is_hostname(const std::string& value) noexcept
{
  return net::is_hostname_valid(value);
}

inline void validate(const bool condition, const std::string& option_name)
{
  if (!condition)
    throw Exception{"invalid value of \"" + option_name +
      "\" dmitigr::ws::Server option"};
}

} // inline namespace validators

/// The representation of Server_options.
class iServer_options final {
public:
  void set_host(std::optional<std::string> value)
  {
    if (value)
      validate(is_ip_address(*value) || is_hostname(*value), "host");
    host_ = std::move(value);
  }

  const std::optional<std::string>& host() const noexcept
  {
    return host_;
  }

  void set_port(std::optional<int> value)
  {
    if (value)
      validate(is_valid_port(*value), "Network port");
    port_ = std::move(value);
  }

  std::optional<int> port() const noexcept
  {
    return port_;
  }

  void set_http_enabled(const std::optional<bool> value)
  {
    is_http_enabled_ = value;
  }

  std::optional<bool> is_http_enabled() const noexcept
  {
    return is_http_enabled_;
  }

  void set_ws_idle_timeout(std::optional<std::chrono::seconds> value)
  {
    if (value)
      validate(value->count() == 0 || (value->count() >= 8 && !(value->count() % 4)),
        "WebSocket idle timeout");
    ws_idle_timeout_ = std::move(value);
  }

  std::optional<std::chrono::seconds> ws_idle_timeout() const noexcept
  {
    return ws_idle_timeout_;
  }

  void set_ws_max_incoming_payload_size(const std::optional<std::size_t> value)
  {
    if (value)
      validate(*value <= std::numeric_limits<int>::max(),
        "WebSocket max incoming payload size");
    ws_max_incoming_payload_size_ = value;
  }

  std::optional<std::size_t> ws_max_incoming_payload_size() const noexcept
  {
    return ws_max_incoming_payload_size_;
  }

  void set_ws_backpressure_buffer_size(const std::optional<std::size_t> value)
  {
    if (value)
      validate(*value <= std::numeric_limits<int>::max(),
        "WebSocket backpressure buffer size");
    ws_backpressure_buffer_size_ = value;
  }

  std::optional<std::size_t> ws_backpressure_buffer_size() const noexcept
  {
    return ws_backpressure_buffer_size_;
  }

  void set_ssl_enabled(const std::optional<bool> value)
  {
#ifndef DMITIGR_LIBS_OPENSSL
    if (value)
      throw Exception{"dmitigr::ws must be compiled with "
        "DMITIGR_LIBS_OPENSSL in order to enable SSL"};
#endif
    is_ssl_enabled_ = value;
  }

  std::optional<bool> is_ssl_enabled() const noexcept
  {
    return is_ssl_enabled_;
  }

  void set_ssl_pem_file_password(std::optional<std::string> value)
  {
    if (value)
      validate(!value->empty(), "SSL PEM file password");
    ssl_pem_file_password_ = std::move(value);
  }

  const std::optional<std::string>& ssl_pem_file_password() const noexcept
  {
    return ssl_pem_file_password_;
  }

  void set_ssl_certificate_file(std::optional<std::filesystem::path> value)
  {
    if (value)
      validate(!value->empty(), "SSL certificate file");
    ssl_certificate_file_ = std::move(value);
  }

  const std::optional<std::filesystem::path>& ssl_certificate_file() const noexcept
  {
    return ssl_certificate_file_;
  }

  void set_ssl_private_key_file(std::optional<std::filesystem::path> value)
  {
    if (value)
      validate(!value->empty(), "SSL private key file");
    ssl_private_key_file_ = std::move(value);
  }

  const std::optional<std::filesystem::path>& ssl_private_key_file() const noexcept
  {
    return ssl_private_key_file_;
  }

  void set_ssl_certificate_authority_file(std::optional<std::filesystem::path> value)
  {
    if (value)
      validate(!value->empty(), "SSL certificate authority file");
    ssl_certificate_authority_file_ = std::move(value);
  }

  const std::optional<std::filesystem::path>&
  ssl_certificate_authority_file() const noexcept
  {
    return ssl_certificate_authority_file_;
  }

  void set_ssl_dh_parameters_file(std::optional<std::filesystem::path> value)
  {
    if (value)
      validate(!value->empty(), "SSL Diffie-Hellman parameters file");
    ssl_dh_parameters_file_ = std::move(value);
  }

  const std::optional<std::filesystem::path>& ssl_dh_parameters_file() const noexcept
  {
    return ssl_dh_parameters_file_;
  }

private:
  friend iServer;
  friend Server_options;

  std::optional<std::string> host_;
  std::optional<int> port_;
  std::optional<std::chrono::seconds> ws_idle_timeout_;
  std::optional<std::size_t> ws_max_incoming_payload_size_;
  std::optional<std::size_t> ws_backpressure_buffer_size_;
  std::optional<bool> is_http_enabled_;
  std::optional<bool> is_ssl_enabled_;
  std::optional<std::string> ssl_pem_file_password_;
  std::optional<std::filesystem::path> ssl_certificate_file_;
  std::optional<std::filesystem::path> ssl_private_key_file_;
  std::optional<std::filesystem::path> ssl_certificate_authority_file_;
  std::optional<std::filesystem::path> ssl_dh_parameters_file_;
};

} // namespace detail

DMITIGR_WS_INLINE Server_options::Server_options()
  : rep_{std::make_unique<detail::iServer_options>()}
{
  DMITIGR_ASSERT(rep_);
}

DMITIGR_WS_INLINE Server_options::Server_options(const Server_options& rhs)
  : rep_{std::make_unique<detail::iServer_options>(*rhs.rep_)}
{
  DMITIGR_ASSERT(rep_);
}

DMITIGR_WS_INLINE Server_options&
Server_options::operator=(const Server_options& rhs)
{
  Server_options tmp{rhs};
  swap(tmp);
  return *this;
}

DMITIGR_WS_INLINE void Server_options::swap(Server_options& other) noexcept
{
  rep_.swap(other.rep_);
}

DMITIGR_WS_INLINE Server_options::~Server_options() noexcept = default;

DMITIGR_WS_INLINE
Server_options::Server_options(Server_options&&) noexcept = default;

DMITIGR_WS_INLINE Server_options&
Server_options::operator=(Server_options&&) noexcept = default;

DMITIGR_WS_INLINE Server_options&
Server_options::set_host(std::optional<std::string> value)
{
  rep_->set_host(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::string>&
Server_options::host() const noexcept
{
  return rep_->host();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_port(const std::optional<int> value)
{
  rep_->set_port(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<int> Server_options::port() const noexcept
{
  return rep_->port();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_http_enabled(const std::optional<bool> value)
{
  rep_->set_http_enabled(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<bool>
Server_options::is_http_enabled() const noexcept
{
  return rep_->is_http_enabled();
}

DMITIGR_WS_INLINE Server_options& Server_options::set_ws_idle_timeout(
  const std::optional<std::chrono::seconds> value)
{
  rep_->set_ws_idle_timeout(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<std::chrono::seconds>
Server_options::ws_idle_timeout() const noexcept
{
  return rep_->ws_idle_timeout();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ws_max_incoming_payload_size(
  const std::optional<std::size_t> value)
{
  rep_->set_ws_max_incoming_payload_size(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<std::size_t>
Server_options::ws_max_incoming_payload_size() const noexcept
{
  return rep_->ws_max_incoming_payload_size();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ws_backpressure_buffer_size(
  const std::optional<std::size_t> value)
{
  rep_->set_ws_backpressure_buffer_size(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<std::size_t>
Server_options::ws_backpressure_buffer_size() const noexcept
{
  return rep_->ws_backpressure_buffer_size();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_enabled(const std::optional<bool> value)
{
  rep_->set_ssl_enabled(value);
  return *this;
}

DMITIGR_WS_INLINE std::optional<bool>
Server_options::is_ssl_enabled() const noexcept
{
  return rep_->is_ssl_enabled();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_pem_file_password(std::optional<std::string> value)
{
  rep_->set_ssl_pem_file_password(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::string>&
Server_options::ssl_pem_file_password() const noexcept
{
  return rep_->ssl_pem_file_password();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_certificate_file(
  std::optional<std::filesystem::path> value)
{
  rep_->set_ssl_certificate_file(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::filesystem::path>&
Server_options::ssl_certificate_file() const noexcept
{
  return rep_->ssl_certificate_file();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_private_key_file(
  std::optional<std::filesystem::path> value)
{
  rep_->set_ssl_private_key_file(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::filesystem::path>&
Server_options::ssl_private_key_file() const noexcept
{
  return rep_->ssl_private_key_file();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_certificate_authority_file(
  std::optional<std::filesystem::path> value)
{
  rep_->set_ssl_certificate_authority_file(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::filesystem::path>&
Server_options::ssl_certificate_authority_file() const noexcept
{
  return rep_->ssl_certificate_authority_file();
}

DMITIGR_WS_INLINE Server_options&
Server_options::set_ssl_dh_parameters_file(
  std::optional<std::filesystem::path> value)
{
  rep_->set_ssl_dh_parameters_file(std::move(value));
  return *this;
}

DMITIGR_WS_INLINE const std::optional<std::filesystem::path>&
Server_options::ssl_dh_parameters_file() const noexcept
{
  return rep_->ssl_dh_parameters_file();
}

} // namespace dmitigr::ws
