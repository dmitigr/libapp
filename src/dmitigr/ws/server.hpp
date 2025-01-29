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

#ifndef DMITIGR_WS_SERVER_HPP
#define DMITIGR_WS_SERVER_HPP

#include "basics.hpp"
#include "server_options.hpp"
#include "dll.hpp"
#include "types_fwd.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace dmitigr::ws {

/// A WebSocket server.
class Server {
public:
  /// Alias of Server_options.
  using Options = Server_options;

  /// The destructor.
  virtual DMITIGR_WS_API ~Server() noexcept;

  /**
   * @brief The constructor.
   *
   * @param loop The event loop.
   * @param options The server options. The default address and port are
   * "0.0.0.0" and 80 correspondingly.
   *
   * @par Requires
   * `loop`.
   *
   * @remarks The server doesn't owns the `loop`.
   */
  explicit DMITIGR_WS_API Server(void* loop, Options options = {});

  /**
   * @returns Options of the server.
   *
   * @par Thread safety
   * Thread-safe.
   */
  DMITIGR_WS_API const Options& options() const noexcept;

  /**
   * @brief Schedules the `callback` to be called on the thread of the event
   * loop associated with this server.
   *
   * @par Thread safety
   * Thread-safe.
   */
  DMITIGR_WS_API void loop_submit(std::function<void()> callback) noexcept;

  /**
   * @returns The native handle of the event loop.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   *
   * @see loop_submit().
   */
  DMITIGR_WS_API const void* loop() const noexcept;

  /// @overload
  DMITIGR_WS_API void* loop() noexcept;

  /**
   * @returns `true` if open for incoming connections.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   */
  DMITIGR_WS_API bool is_started() const noexcept;

  /**
   * @brief Starts the server.
   *
   * @details Starts the loop(), blocks the thread.
   *
   * @par Thread safety
   * NOT thread-safe.
   *
   * @par Effects
   * `is_started()`.
   */
  DMITIGR_WS_API void start();

  /**
   * @brief Stops the server.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   */
  DMITIGR_WS_API void stop() noexcept;

  /**
   * @brief Closes all the open WebSocket connections.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   */
  DMITIGR_WS_API void close_connections(int code, std::string reason) noexcept;

  /**
   * @brief Walks over the open WebSocket connections.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   */
  DMITIGR_WS_API void walk(std::function<void(Connection&)> callback);

  /**
   * @returns The number of open connections.
   *
   * @par Thread safety
   * Thread-safe.
   *
   * @remarks The behaviour is undefined if called not on the thread of the
   * associated event loop!
   */
  DMITIGR_WS_API std::size_t connection_count() const noexcept;

private:
  /**
   * @brief This function is called on every opening handshake (HTTP Upgrade
   * request) from a WebSocket client.
   *
   * @details Handshake from the server may be either implicitly rejected (HTTP
   * response with status code of `500` will be send to the client) or implicitly
   * completed by just returning `nullptr` or a new Connection instance
   * accordingly. Handshake from the server may be deferred by setting abort
   * handler on `io` and returning `nullptr` or a new Connection instance.
   * Deferred handshake can be either explicitly rejected or explicitly completed
   * later by sending a custom HTTP response or by calling Http_io::end_handshake()
   * method accordingly.
   *
   * @param req A HTTP request.
   * @param io An IO object which should be touched only in cases of explicit
   * rejections or completions (either deferred or not). To defer the handshake
   * the abort handler must be set on this object.
   *
   * @par Postconditions
   *   -# For any handshake completion: `io->is_valid()`;
   *   -# For implicit handshake completion: `!io->is_response_handler_set()`.
   *
   * @returns The result may be:
   *   - `nullptr` to reject the handshake implicitly (abort handler must not be
   *   set);
   *   - `nullptr` to reject the handshake explicitly (abort handler must be set);
   *   - a new Connection instance to complete handshake implicitly or to defer
   *   the handshake (for either rejection or completion).
   *
   * @throws Exception if any of postconditions are violated.
   *
   * @remarks This function is called on the thread of the associated event loop.
   * @remarks The behaviour is undefined if `io` has been used for sending any
   * data from within this function in cases of implicit rejection or completion
   * of the handshake.
   *
   * @see Http_io.
   */
  virtual std::shared_ptr<Connection> handle_handshake(const Http_request& req,
    std::shared_ptr<Http_io> io) noexcept = 0;

  /**
   * @brief This function to be called on every HTTP request if HTTP functionality
   * is enabled in Server_options.
   *
   * @param req A HTTP request.
   * @param io An IO object To defer the handshake the abort handler must be set
   * on this object.
   *
   * @remarks This function is called on the thread of the associated event loop.
   * @remarks `io` will be aborted in case if it's unfinished and it doesn't have
   * the abort handler set immediately after returning from this function.
   *
   * @see Server_options.
   */
  virtual void handle_request(const Http_request& req,
    std::shared_ptr<Http_io> io) noexcept = 0;

private:
  friend detail::iServer;
  template<bool> friend class detail::Srv;

  mutable std::mutex mut_;
  std::unique_ptr<detail::iServer> rep_;
};

} // namespace dmitigr::ws

#ifndef DMITIGR_WS_NOT_HEADER_ONLY
#include "server.cpp"
#endif

#endif  // DMITIGR_WS_SERVER_HPP
