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

#ifndef DMITIGR_WS_CONNECTION_HPP
#define DMITIGR_WS_CONNECTION_HPP

#include "../net/address.hpp"
#include "dll.hpp"
#include "types_fwd.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace dmitigr::ws {

/**
 * @brief A WebSocket connection.
 *
 * @remarks The API is suitable for both client and server. But client
 * connections are not supported by the library at the moment.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
  /// The destructor.
  virtual DMITIGR_WS_API ~Connection() noexcept;

  /// The default-constructible.
  DMITIGR_WS_API Connection() noexcept;

  /**
   * @brief Attempts to schedule the `callback` to the called on the thread of
   * the event loop associated with this instance.
   *
   * @returns `true` on success.
   *
   * @par Thread safety
   * Thread-safe.
   */
  DMITIGR_WS_API bool loop_submit(std::function<void()> callback) noexcept;

  /**
   * @returns The server associated with this instance. (If the instance were a
   * client connection, then the return value would be `nullptr`.)
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @see loop_submit().
   */
  virtual DMITIGR_WS_API const Server* server() const noexcept;

  /// @overload
  virtual DMITIGR_WS_API Server* server() noexcept;

  /**
   * @returns `true` if the underlying socket is valid.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @see loop_submit().
   */
  DMITIGR_WS_API bool is_connected() const noexcept;

  /**
   * @returns The valid remote IP address.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @see loop_submit(), local_ip_address().
   */
  DMITIGR_WS_API const net::Ip_address& remote_ip_address() const noexcept;

  /**
   * @returns The valid local IP address.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @see loop_submit(), remote_ip_address().
   */
  DMITIGR_WS_API const net::Ip_address& local_ip_address() const noexcept;

  /**
   * @returns The amount of bytes of data that have been buffered (queued) due
   * to backpressure provoked by send() but not yet transmitted to the network.
   * This value tends to zero as pending data is transmitted. Returns zero
   * if `!is_connected()`.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit(), send(), handle_drain(),
   * Server_options::ws_backpressure_buffer_size().
   */
  DMITIGR_WS_API std::size_t buffered_amount() const noexcept;

  /**
   * @brief Attempts the specified `payload` of the specified `format` to be
   * transmitted to the remote side over the connection.
   *
   * @details In case of backpressure the `payload` will be queued into the
   * *backpressure buffer*. When the pending data is actually transmitted over
   * the network, function handle_drain() will be called.
   *
   * @returns `true` if the `payload` is actually transmitted, or `false` if
   * backpressure case occurred and the `payload` (or it's part) was queued into
   * the backpressure buffer to be transmitted as soon as possible.
   *
   * @par Requires
   * `is_connected()`.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @warning Before sending more data, the value returned by either this method
   * and/or by method buffered_amount() should be taken into account to avoid
   * possible backpressure buffer (or even **system memory**) exhaustion!
   *
   * @see loop_submit(), send_utf8(), send_binary(), buffered_amount(),
   * handle_drain().
   */
  DMITIGR_WS_API bool send(std::string_view payload, Data_format format);

  /// @returns send(payload, Data_format::utf8).
  DMITIGR_WS_API bool send_utf8(std::string_view payload);

  /// @returns send(payload, Data_format::binary).
  DMITIGR_WS_API bool send_binary(std::string_view payload);

  /**
   * @brief Closes the connection in a normal way.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   * @remarks Has no effect if `!is_connected()`.
   *
   * @see loop_submit(), handle_close(), abort().
   */
  DMITIGR_WS_API void close(int code, std::string_view reason) noexcept;

  /**
   * @brief Closes the connection immediately (abnormally).
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   * @remarks Has no effect if `!is_connected()`.
   *
   * @see loop_submit(), close().
   */
  DMITIGR_WS_API void abort() noexcept;

private:
  /**
   * @brief This function is called when connection is open.
   *
   * @remarks This function is called on the thread of the associated event loop.
   * @remarks When this function is called `is_connected() == true`.
   *
   * @see handle_close().
   */
  virtual void handle_open() noexcept = 0;

  /**
   * @brief This function is called when message is incoming from a remote side.
   *
   * @remarks This function is called on the thread of the associated event loop.
   *
   * @see Server_options::set_ws_max_incoming_payload_size().
   */
  virtual void handle_message(std::string_view payload, Data_format format) noexcept = 0;

  /**
   * @brief This function is called when buffered due to backpressure data (or
   * it's part) was transmitted over the network.
   *
   * @details The call of this function signals that the buffered amount has
   * reduced since the last call of send(), allowing to prevent backpressure
   * buffer (or even **system memory**) exhaustion with sending new data.
   *
   * @remarks This function is called on the thread of the associated event loop.
   *
   * @see buffered_amount().
   */
  virtual void handle_drain() noexcept = 0;

  /**
   * @brief This function is called when connection is (about to be) closed.
   *
   * @remarks This function is called on the thread of the associated event loop.
   * @remarks The underlying socket may be destroyed before the moment of this
   * function call. Hence, the implementation must use `is_connected()` before
   * using the API which requires `is_connected()` as a precondition.
   *
   * @see close().
   */
  virtual void handle_close(int code, std::string_view reason) noexcept = 0;

private:
  friend detail::iServer;
  template<bool> friend class detail::Srv;

  mutable std::mutex mut_;
  std::unique_ptr<detail::iConnection> rep_;

  bool is_connected_nts() const noexcept;
};

} // namespace dmitigr::ws

#ifndef DMITIGR_WS_NOT_HEADER_ONLY
#include "connection.cpp"
#endif

#endif  // DMITIGR_WS_CONNECTION_HPP
