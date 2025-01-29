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

#ifndef DMITIGR_WSCL_CONNECTION_HPP
#define DMITIGR_WSCL_CONNECTION_HPP

#include "basics.hpp"
#include "connection_options.hpp"
#include "dll.hpp"
#include "../3rdparty/uwsc/uwsc.h"

#include <memory>

namespace dmitigr::wscl {

/**
 * @brief WebSocket connection.
 *
 * @see Connection_options.
 */
class Connection {
public:
  /// An alias of Connection_options.
  using Options = Connection_options;

  /// Closes connection with normal status.
  virtual DMITIGR_WSCL_API ~Connection();

  /// Default-constructible.
  Connection() = default;

  /**
   * @brief Constructs an instance and attaches it to the specifed `loop`.
   *
   * @par Requires
   * `loop`.
   *
   * @see reset().
   */
  DMITIGR_WSCL_API Connection(uwsc_loop* loop, Options options);

  /**
   * @brief Resets the instance by attaching it to the specified loop.
   *
   * @par Requires
   * `loop`.
   */
  DMITIGR_WSCL_API void reset(uwsc_loop* loop, Options options);

  /// @returns The event loop.
  DMITIGR_WSCL_API const uwsc_loop* loop() const noexcept;

  /// @overload
  DMITIGR_WSCL_API uwsc_loop* loop() noexcept;

  /**
   * @returns `true` if the connection is open.
   *
   * @remarks Starts return `true` just before call of handle_open().
   *
   * @see handle_open().
   */
  DMITIGR_WSCL_API bool is_open() const noexcept;

  /// @returns Connection options.
  DMITIGR_WSCL_API const Options& options() const noexcept;

  /**
   * @brief Sets the ping interval on open connection. (Overwrites
   * options().ping_interval().)
   *
   * @par Requires
   * `is_open()`.
   */
  DMITIGR_WSCL_API void set_ping_interval(std::chrono::seconds interval);

  /**
   * @brief Sends the data of specified format to the WebSocket server.
   *
   * @par Requires
   * `is_open()`.
   */
  DMITIGR_WSCL_API void send(std::string_view data, Data_format format);

  /**
   * @brief Sends the text data to the WebSocket server.
   *
   * @par Requires
   * `is_open()`.
   */
  DMITIGR_WSCL_API void send_utf8(std::string_view data);

  /**
   * @brief Sends the binary data to the WebSocket server.
   *
   * @par Requires
   * `is_open()`.
   */
  DMITIGR_WSCL_API void send_binary(std::string_view data);

  /**
   * @brief Pings the WebSocket server.
   *
   * @par Requires
   * `is_open()`.
   */
  DMITIGR_WSCL_API void ping();

  /**
   * @brief Initiates connection close.
   *
   * @remarks is_open() starts return `false` only just before call of
   * handle_close().
   */
  DMITIGR_WSCL_API void close(int code, const std::string& reason = {}) noexcept;

private:
  /// @name Callbacks
  /// @remarks These functions are called on the thread of the associated event
  /// loop.
  /// @{

  /// This function is called just after successful connection open.
  virtual void handle_open() noexcept = 0;

  /// This function is called just after message is received.
  virtual void handle_message(std::string_view data, Data_format format) noexcept = 0;

  /// This function is called on error.
  virtual void handle_error(int code, std::string_view message) noexcept = 0;

  /// This function is called when the underlying socket is about to be close.
  virtual void handle_close(int code, std::string_view reason) noexcept = 0;

  /// @}

private:
  struct Rep;
  std::unique_ptr<Rep> rep_;

  static void handle_open__(uwsc_client* cl) noexcept;

  static void handle_message__(uwsc_client* cl, void* data, std::size_t size,
    bool binary) noexcept;

  static void handle_error__(uwsc_client* cl, int code,
    const char* message) noexcept;

  static void handle_close__(uwsc_client* cl, int code,
    const char* reason) noexcept;

  static Connection* self(uwsc_client* cl) noexcept;
};

} // namespace dmitigr::wscl

#ifndef DMITIGR_WSCL_NOT_HEADER_ONLY
#include "connection.cpp"
#endif

#endif  // DMITIGR_WSCL_CONNECTION_HPP
