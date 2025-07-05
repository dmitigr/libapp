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

#ifndef DMITIGR_WSCL_CONNECTION_OPTIONS_HPP
#define DMITIGR_WSCL_CONNECTION_OPTIONS_HPP

#include "dll.hpp"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace dmitigr::wscl {

/**
 * @brief WebSocket connection options.
 *
 * @see Connection.
 */
class Connection_options final {
public:
  /**
   * @brief Sets the address of remote WebSocket server.
   *
   * @param value A valid hostname or IP address. `std::nullopt` means `127.0.0.1`.
   *
   * @par Requires
   * `!value` or `*value` is a valid hostname or IP address.
   */
  DMITIGR_WSCL_API Connection_options& set_host(std::optional<std::string> value);

  /// @returns The address of remote WebSocket server.
  DMITIGR_WSCL_API const std::optional<std::string>& host() const noexcept;

  /**
   * @brief Sets the port of remote WebSocket server.
   *
   * @param value The port number to use for binding on. `std::nullopt` means `80`.
   *
   * @par Requires
   * `!value` or `*value` is a valid port.
   */
  DMITIGR_WSCL_API Connection_options& set_port(std::optional<int> value);

  /// @returns The port of remote WebSocket server.
  DMITIGR_WSCL_API std::optional<int> port() const noexcept;

  /**
   * @returns The URL string.
   *
   * @throw Exception if any of host or port is not specified.
   */
  DMITIGR_WSCL_API std::string url() const;

  /**
   * @brief Sets the ping interval.
   *
   * @remarks `value` less or equals to `0` disables pings.
   */
  DMITIGR_WSCL_API Connection_options&
  set_ping_interval(std::optional<std::chrono::seconds> value);

  /// @returns The current ping interval.
  DMITIGR_WSCL_API std::optional<std::chrono::seconds>
  ping_interval() const noexcept;

  /// Adds the extra header to pass upon handshake.
  DMITIGR_WSCL_API Connection_options&
  set_extra_header(std::string_view name, std::string_view value);

  /// @returns The extra headers to pass upon handshake.
  DMITIGR_WSCL_API std::string extra_headers_string() const;

  /// @name SSL options
  /// @{

  /**
   * @brief Enables the SSL mode if `value && *value`.
   *
   * @param value `std::nullopt` or `false` means *disabled*.
   *
   * @par Requires
   * The library must be compiled with DMITIGR_LIBS_OPENSSL.
   *
   * @remarks SSL mode is disabled by default.
   *
   * @see is_ssl_enabled().
   */
  DMITIGR_WSCL_API Connection_options& set_ssl_enabled(std::optional<bool> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_enabled().
   */
  DMITIGR_WSCL_API std::optional<bool> is_ssl_enabled() const;

  /**
   * @brief Sets the name of the file containing a SSL client certificate.
   *
   * @par Requires
   * `(*is_ssl_enabled() && (!value || !value->empty()))`.
   *
   * @see ssl_certificate_file().
   */
  DMITIGR_WSCL_API Connection_options&
  set_ssl_certificate_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_certificate_file().
   */
  DMITIGR_WSCL_API const std::optional<std::filesystem::path>&
  ssl_certificate_file() const;

  /**
   * @brief Sets the name of the file containing a SSL client private key.
   *
   * @par Requires
   * `(*is_ssl_enabled() && (!value || !value->empty()))`.
   *
   * @see ssl_private_key_file().
   */
  DMITIGR_WSCL_API Connection_options&
  set_ssl_private_key_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_private_key_file().
   */
  DMITIGR_WSCL_API const std::optional<std::filesystem::path>&
  ssl_private_key_file() const;

  /**
   * @brief Sets the name of the file containing a SSL client certificate
   * authority (CA).
   *
   * @details If this option is set, a verification that the WebSocket server
   * certificate is issued by a trusted certificate authority (CA) will be
   * performed.
   *
   * @par Requires
   * `(*is_ssl_enabled() && (!value || !value->empty()))`.
   *
   * @see ssl_certificate_authority_file().
   */
  DMITIGR_WSCL_API Connection_options&
  set_ssl_certificate_authority_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_certificate_authority_file().
   */
  DMITIGR_WSCL_API const std::optional<std::filesystem::path>&
  ssl_certificate_authority_file() const;

  /// @}

private:
  std::optional<std::string> host_;
  std::optional<int> port_{};
  std::optional<std::chrono::seconds> ping_interval_;
  std::optional<bool> is_ssl_enabled_;
  std::optional<std::filesystem::path> set_ssl_certificate_file_;
  std::optional<std::filesystem::path> ssl_certificate_file_;
  std::optional<std::filesystem::path> ssl_private_key_file_;
  std::optional<std::filesystem::path> ssl_certificate_authority_file_;
  std::string extra_headers_;
};

} // namespace dmitigr::wscl

#ifndef DMITIGR_WSCL_NOT_HEADER_ONLY
#include "connection_options.cpp"
#endif

#endif  // DMITIGR_WSCL_CONNECTION_OPTIONS_HPP
