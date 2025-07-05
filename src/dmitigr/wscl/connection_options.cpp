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

#include "../net/address.hpp"
#include "../net/util.hpp"
#include "connection_options.hpp"
#include "exceptions.hpp"

namespace dmitigr::wscl {
namespace detail {

template<typename T>
inline bool is_valid_port(const T value) noexcept
{
  return 0 < value && value < 65536;
}

inline bool is_ip_address(const std::string& value)
{
  return net::Ip_address::is_valid(value);
}

inline bool is_hostname(const std::string& value)
{
  return net::is_hostname_valid(value);
}

inline void validate(const bool condition, const std::string& option_name)
{
  if (!condition)
    throw Exception{"invalid value of \"" + option_name +
      "\" WebSocket client option"};
}

} // namespace detail

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_host(std::optional<std::string> value)
{
  using namespace detail;
  if (value)
    validate(is_ip_address(*value) || is_hostname(*value), "host");
  host_ = std::move(value);
  return *this;
}

DMITIGR_WSCL_INLINE const std::optional<std::string>&
Connection_options::host() const noexcept
{
  return host_;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_port(const std::optional<int> value)
{
  using namespace detail;
  if (value)
    validate(is_valid_port(*value), "port");
  port_ = value;
  return *this;
}

DMITIGR_WSCL_INLINE std::optional<int> Connection_options::port() const noexcept
{
  return port_;
}

DMITIGR_WSCL_INLINE std::string Connection_options::url() const
{
  std::string result{is_ssl_enabled() && *is_ssl_enabled() ? "wss://" : "ws://"};
  result.append(host().value_or("127.0.0.1"));
  result.append(":").append(std::to_string(port().value_or(80)));
  return result;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_ping_interval(
  const std::optional<std::chrono::seconds> value)
{
  ping_interval_ = value;
  return *this;
}

DMITIGR_WSCL_INLINE std::optional<std::chrono::seconds>
Connection_options::ping_interval() const noexcept
{
  return ping_interval_;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_extra_header(const std::string_view name,
  const std::string_view value)
{
  extra_headers_.append(name).append(":").append(value).append("\r\n");
  return *this;
}

DMITIGR_WSCL_INLINE std::string
Connection_options::extra_headers_string() const
{
  return extra_headers_;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_ssl_enabled(const std::optional<bool> value)
{
#ifndef DMITIGR_LIBS_OPENSSL
  if (value)
    throw Exception{"dmitigr::wscl must be compiled with "
      "DMITIGR_LIBS_OPENSSL in order to enable SSL"};
#endif
  is_ssl_enabled_ = value;
  return *this;
}

DMITIGR_WSCL_INLINE std::optional<bool>
Connection_options::is_ssl_enabled() const
{
  return is_ssl_enabled_;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_ssl_certificate_file(
  std::optional<std::filesystem::path> value)
{
  if (value)
    detail::validate(!value->empty(), "SSL certificate file");
  ssl_certificate_file_ = std::move(value);
  return *this;
}

DMITIGR_WSCL_INLINE const std::optional<std::filesystem::path>&
Connection_options::ssl_certificate_file() const
{
  return ssl_certificate_file_;
}

DMITIGR_WSCL_INLINE Connection_options&
Connection_options::set_ssl_private_key_file(
  std::optional<std::filesystem::path> value)
{
  if (value)
    detail::validate(!value->empty(), "SSL private key file");
  ssl_private_key_file_ = std::move(value);
  return *this;
}

DMITIGR_WSCL_INLINE const std::optional<std::filesystem::path>&
Connection_options::ssl_private_key_file() const
{
  return ssl_private_key_file_;
}

DMITIGR_WSCL_INLINE  Connection_options&
Connection_options::set_ssl_certificate_authority_file(
  std::optional<std::filesystem::path> value)
{
  if (value)
    detail::validate(!value->empty(), "SSL certificate authority file");
  ssl_certificate_authority_file_ = std::move(value);
  return *this;
}

DMITIGR_WSCL_INLINE const std::optional<std::filesystem::path>&
Connection_options::ssl_certificate_authority_file() const
{
  return ssl_certificate_authority_file_;
}

} // namespace dmitigr::wscl
