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

#ifndef DMITIGR_WS_SERVER_OPTIONS_HPP
#define DMITIGR_WS_SERVER_OPTIONS_HPP

#include "dll.hpp"
#include "types_fwd.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace dmitigr::ws {

/// A WebSocket Server options.
class Server_options final {
public:
    /// The destructor.
  DMITIGR_WS_API ~Server_options() noexcept;

  /// @name Constructors
  /// @{

  /// Constructs the default server options.
  DMITIGR_WS_API Server_options();

  /// Copy-constructible.
  DMITIGR_WS_API Server_options(const Server_options& rhs);

  /// Copy-assignable.
  DMITIGR_WS_API Server_options& operator=(const Server_options& rhs);

  /// Move-constructible.
  DMITIGR_WS_API Server_options(Server_options&& rhs) noexcept;

  /// Move-assignable.
  DMITIGR_WS_API Server_options& operator=(Server_options&& rhs) noexcept;

  /// Swaps `*this` with `other`.
  DMITIGR_WS_API void swap(Server_options& other) noexcept;

  /// @}

  /**
   * @brief Sets the host for binding.
   *
   * @param value A valid hostname or IP address.
   * `std::nullopt` means `0.0.0.0`.
   *
   * @par Requires
   * `!value` or `*value` is a valid IP address.
   */
  DMITIGR_WS_API Server_options& set_host(std::optional<std::string> value);

  /// @returns The current value of the option.
  DMITIGR_WS_API const std::optional<std::string>& host() const noexcept;

  /**
   * @brief Sets the port for binding.
   *
   * @param value The port number to use for binding on.
   * `std::nullopt` means `80`.
   *
   * @par Requires
   * `!value` or `*value` is a valid port.
   */
  DMITIGR_WS_API Server_options& set_port(std::optional<int> value);

  /// @returns The current value of the option.
  DMITIGR_WS_API std::optional<int> port() const noexcept;

  /**
   * @brief Enables the HTTP mode if `value && *value`.
   *
   * @param value `std::nullopt` or `false` means *disabled*.
   *
   * @remarks HTTP mode is disabled by default.
   *
   * @see is_ssl_enabled().
   */
  DMITIGR_WS_API Server_options& set_http_enabled(std::optional<bool> value);

  /// @returns `true` if the HTTP mode is enabled.
  DMITIGR_WS_API std::optional<bool> is_http_enabled() const noexcept;

  /**
   * @brief Sets the timeout of the idle WebSocket connection.
   *
   * @param value `std::nullopt` or `0` means *eternity*.
   *
   * @par Requires
   * `!value || (value.count() == 0 || (value.count() >= 8 && !(value.count() % 4)))`.
   *
   * @see idle_timeout().
   */
  DMITIGR_WS_API Server_options&
  set_ws_idle_timeout(std::optional<std::chrono::seconds> value);

  /**
   * @return The current value of the idle timeout of a WebSocket connection.
   *
   * @see set_ws_idle_timeout().
   */
  DMITIGR_WS_API std::optional<std::chrono::seconds> ws_idle_timeout() const noexcept;

  /**
   * @brief Sets the maximum size of incoming WebSocket message payload.
   *
   * @param value `std::nullopt` or `0` means *max possible*.
   *
   * @par Requires
   * `!value || (value <= std::numeric_limits<int>::max())`.
   *
   * @see max_ws_incoming_payload_size(), Connection::handle_message().
   */
  DMITIGR_WS_API Server_options&
  set_ws_max_incoming_payload_size(std::optional<std::size_t> value);

  /**
   * @return The current value of the maximum size of incoming WebSocket
   * message payload.
   *
   * @see set_ws_max_incoming_payload_size().
   */
  DMITIGR_WS_API std::optional<std::size_t>
  ws_max_incoming_payload_size() const noexcept;

  /**
   * @brief Sets the WebSocket backpressure buffer size.
   *
   * @param value `std::nullopt` or `0` means *max possible*.
   *
   * @par Requires
   * `!value || (value <= std::numeric_limits<int>::max())`.
   *
   * @see ws_backpressure_buffer_size(), Connection::handle_drain().
   */
  DMITIGR_WS_API Server_options&
  set_ws_backpressure_buffer_size(std::optional<std::size_t> value);

  /**
   * @return The current value of the WebSocket backpressure buffer size.
   *
   * @see set_ws_backpressure_buffer_size(), Connection::buffered_amount().
   */
  DMITIGR_WS_API std::optional<std::size_t>
  ws_backpressure_buffer_size() const noexcept;

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
  DMITIGR_WS_API Server_options& set_ssl_enabled(std::optional<bool> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_enabled().
   */
  DMITIGR_WS_API std::optional<bool> is_ssl_enabled() const noexcept;

  /**
   * @brief Sets the password for encrypted PEM file.
   *
   * @par Requires
   * `*is_ssl_enabled() && (!value || !value->empty())`.
   *
   * @see ssl_pem_file_password().
   */
  DMITIGR_WS_API Server_options&
  set_ssl_pem_file_password(std::optional<std::string> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_pem_file_password().
   */
  DMITIGR_WS_API const std::optional<std::string>&
  ssl_pem_file_password() const noexcept;

  /**
   * @brief Sets the name of the file containing a SSL client certificate.
   *
   * @par Requires
   * `*is_ssl_enabled() && (!value || !value->empty())`.
   *
   * @see ssl_certificate_file().
   */
  DMITIGR_WS_API Server_options&
  set_ssl_certificate_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_certificate_file().
   */
  DMITIGR_WS_API const std::optional<std::filesystem::path>&
  ssl_certificate_file() const noexcept;

  /**
   * @brief Sets the name of the file containing a SSL client private key.
   *
   * @par Requires
   * `*is_ssl_enabled() && (!value || !value->empty())`.
   *
   * @see ssl_private_key_file().
   */
  DMITIGR_WS_API Server_options&
  set_ssl_private_key_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_private_key_file().
   */
  DMITIGR_WS_API const std::optional<std::filesystem::path>&
  ssl_private_key_file() const noexcept;

  /**
   * @brief Sets the name of the file containing a SSL client certificate
   * authority (CA).
   *
   * @details If this option is set, a verification that the WebSocket server
   * certificate is issued by a trusted certificate authority (CA) will be
   * performed.
   *
   * @par Requires
   * `*is_ssl_enabled() && (!value || !value->empty())`.
   *
   * @see ssl_certificate_authority_file().
   */
  DMITIGR_WS_API Server_options&
  set_ssl_certificate_authority_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_certificate_authority_file().
   */
  DMITIGR_WS_API const std::optional<std::filesystem::path>&
  ssl_certificate_authority_file() const noexcept;

  /**
   * @brief Sets the name of the file containing Diffie-Hellman parameters.
   *
   * @par Requires
   * `*is_ssl_enabled() && (!value || !value->empty())`.
   *
   * @see ssl_dh_parameters_file().
   */
  DMITIGR_WS_API Server_options&
  set_ssl_dh_parameters_file(std::optional<std::filesystem::path> value);

  /**
   * @returns The current value of the option.
   *
   * @see set_ssl_dh_parameters_file().
   */
  DMITIGR_WS_API const std::optional<std::filesystem::path>&
  ssl_dh_parameters_file() const noexcept;

  /// @}

private:
  std::unique_ptr<detail::iServer_options> rep_;
};

} // namespace dmitigr::ws

#ifndef DMITIGR_WS_NOT_HEADER_ONLY
#include "server_options.cpp"
#endif

#endif  // DMITIGR_WS_SERVER_OPTIONS_HPP
