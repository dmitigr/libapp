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

#ifndef DMITIGR_WS_HTTP_IO_HPP
#define DMITIGR_WS_HTTP_IO_HPP

#include "../http/types_fwd.hpp"
#include "types_fwd.hpp"

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

namespace dmitigr::ws {

/// A HTTP I/O.
class Http_io {
public:
  /**
   * @brief An alias of a function to handle a cases when the communication
   * channel is closed abnormally.
   *
   * @details For example, when the client just closes the browser. Also, this
   * function is called if abort() is called.
   *
   * @see abort(), set_abort_handler().
   */
  using Abort_handler = std::function<void()>;

  /**
   * @brief An alias of a function to handle the incoming content.
   *
   * @param data Portion of incoming data to consume.
   * @param is_last Whether the `data` is the last portion of the incoming data?
   *
   * @see set_receive_handler().
   */
  using Receive_handler = std::function<void(std::string_view data, bool is_last)>;

  /*
   * @brief An alias of a function to handle sending of a content piecewise.
   *
   * @details The parameter `position` denotes a starting position of data to be
   * used for a next call of send_content(). The function must return `true` if
   * send operation was successful.
   *
   * @see set_send_handler().
   */
  using Send_handler = std::function<bool(std::uintmax_t position)>;

  /// The destructor.
  virtual ~Http_io() = default;

  /**
   * @returns `true` if calling of any method other than the destructor or
   * is_valid() will not lead to the undefined behavior.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit().
   */
  virtual bool is_valid() const noexcept = 0;

  /**
   * @brief Attempts to schedule the `callback` to the called on the thread of
   * the event loop associated with this instance.
   *
   * @returns `true` on success.
   *
   * @par Thread safety
   * Thread-safe.
   */
  virtual bool loop_submit(std::function<void()> callback) noexcept = 0;

  /**
   * @returns The server associated with this instance, or `nullptr` if `!is_valid()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit().
   */
  virtual const Server* server() const noexcept = 0;

  /// @overload
  virtual Server* server() noexcept = 0;

  /**
   * @returns A pointer to an instance of a not yet open WebSocket connection
   * immediately after returning from Server::handle_handshake() and until a
   * call of end_handshake(). Returns `nullptr` during calls of
   * Server::handle_handshake() or Server::handle_request().
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), end_handshake(), Server::handle_handshake(),
   * Server::handle_request().
   */
  virtual const Connection* connection() const noexcept = 0;

  /// @overload
  virtual Connection* connection() noexcept = 0;

  /**
   * @brief Sends a WebSocket handshake and finishes the IO.
   *
   * @par Requires
   * `is_valid() && connection()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @par Effects
   * `!is_valid() && !connection()`.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), connection(), Server::handle_handshake().
   */
  virtual void end_handshake() = 0;

  /**
   * @brief Sends the Status-Line.
   *
   * @par Requires
   * `is_valid()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit().
   */
  virtual void send_status(http::Server_errc code) = 0;

  /**
   * @brief Sends a response-header field.
   *
   * @param name The name of header, like "Content-Type".
   * @param value The value of header, like "text/html".
   *
   * @par Requires
   * `is_valid()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit().
   */
  virtual void send_header(std::string_view name, std::string_view value) = 0;

  /**
   * @brief Attempts to send (a portion) of the content.
   *
   * @details If no handler set by set_send_handler() then the entire `data`
   * will be send upon a call which may not be suitable if the data size is huge!
   *
   * @param data Data to send.
   * @param total_size Total size of content to send. `0` implies `data.size()`.
   *
   * @returns A pair of two booleans:
   *   -# indicates success of send operation. If `false` then there is no
   *   readiness to send more data at the moment;
   *   -# indicates whether the `total_size` of bytes were sent in summary
   *   after this call. If `false` then the handler which was set by
   *   set_send_handler() method will be called at the moment of readiness to
   *   send more data.
   *
   * @par Requires
   * `is_valid()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @par Effects
   * If the send handler is not set, then `!is_valid()` after calling this
   * method. Otherwise `!is_valid()` after the send handler returns `true`.
   *
   * @remarks The `Content-Length` header will be included into the message.
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), set_send_handler(), end().
   */
  virtual std::pair<bool, bool> send_content(std::string_view data,
    std::uintmax_t total_size = 0) = 0;

  /**
   * @brief Sends the `data` (if any) and finishes the IO.
   *
   * @par Effects
   * `!is_valid()`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), send_content().
   */
  virtual void end(std::string_view data = {}) = 0;

  /**
   * @brief Sets the send handler.
   *
   * @par Requires
   * `is_valid() && !is_send_handler_set() && handler`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see Send_handler, loop_submit(), send_content().
   */
  virtual void set_send_handler(Send_handler handler) = 0;

  /**
   * @returns `true` if the respond handler was set.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), set_send_handler().
   */
  virtual bool is_send_handler_set() const noexcept = 0;

  /**
   * @brief Closes the communication immediately (abnormally).
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @par Effects
   * `!is_valid()`.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), set_abort_handler().
   */
  virtual void abort() noexcept = 0;

  /**
   * @brief Sets the abort handler.
   *
   * @par Requires
   * `is_valid() && !is_abort_handler_set() && handler`.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), Abort_handler.
   */
  virtual void set_abort_handler(Abort_handler handler) = 0;

  /**
   * @returns `true` if the abort handler was set.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), set_abort_handler().
   */
  virtual bool is_abort_handler_set() const noexcept = 0;

  /**
   * @brief Sets the receive handler.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @par Requires
   * `is_valid() && !is_receive_handler_set() && handler`.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), Receive_handler.
   */
  virtual void set_receive_handler(Receive_handler handler) = 0;

  /**
   * @returns `true` if the receive handler was set.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), set_receive_handler().
   */
  virtual bool is_receive_handler_set() const noexcept = 0;

private:
  friend detail::iHttp_io;

  Http_io() = default;
};

} // namespace dmitigr::ws

#ifndef DMITIGR_WS_NOT_HEADER_ONLY
#include "http_io.cpp"
#endif

#endif  // DMITIGR_WS_HTTP_IO_HPP
