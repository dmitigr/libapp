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

#ifndef DMITIGR_IO_CONNECTION_HPP
#define DMITIGR_IO_CONNECTION_HPP

#include <boost/asio.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace dmitigr::io {

/// A connection.
struct Connection final {
  boost::asio::ip::tcp::socket socket;
};

/// A proxy connection to link with another one.
class Proxy_connection : public std::enable_shared_from_this<Proxy_connection> {
protected:
  struct Must_be_shared_ptr final {};

public:
  /// The destructor.
  virtual ~Proxy_connection() = default;

  /// Constructs new instance.
  Proxy_connection(Must_be_shared_ptr, Connection connection)
    : connection_{std::move(connection)}
  {}

  /// Links this instance with `lconn`.
  void link(std::weak_ptr<Proxy_connection> lconn)
  {
    if (is_linked())
      throw std::logic_error{"cannot link Proxy_connection: already linked"};
    linked_connection_ = lconn;
    if (const auto llconn = linked_connection_.lock())
      llconn->linked_connection_ = shared_from_this();
  }

  /// @returns Connection.
  Connection& connection() noexcept
  {
    return connection_;
  }

  /// @overload
  const Connection& connection() const noexcept
  {
    return connection_;
  }

  /// @retuns Linked connection.
  std::weak_ptr<Proxy_connection> linked_connection() const noexcept
  {
    return linked_connection_;
  }

  /// @returns `true` if this instance is linked.
  bool is_linked() const noexcept
  {
    return !linked_connection_.expired();
  }

  /// Initiates to wait for connection read-ready state.
  void async_wait_read_ready() noexcept
  {
    try {
      if (!connection_.socket.is_open())
        throw std::system_error{make_error_code(std::errc::not_connected)};

      connection_.socket.async_wait(boost::asio::ip::tcp::socket::wait_read,
        [self = shared_from_this()](const std::error_code& error)
        {
          if (error)
            return self->finish(error);
          else if (!self->connection_.socket.available())
            return self->finish(make_error_code(std::errc::connection_aborted));

          try {
            self->handle_read_ready();
          } catch (const boost::system::system_error& e) {
            self->finish(e.code());
          } catch (const std::system_error& e) {
            self->finish(e.code());
          } catch (...) {
            self->finish(make_error_code(std::errc::operation_canceled));
          }
        });
    } catch (const boost::system::system_error& e) {
      finish(e.code());
    } catch (const std::system_error& e) {
      finish(e.code());
    } catch (...) {
      finish(make_error_code(std::errc::operation_canceled));
    }
  }

  /// Initiates to write the `buf` contents to the `linked_connection()`.
  void async_write_to_linked(const boost::asio::const_buffer buf) noexcept
  {
    try {
      if (buf.size()) {
        const auto linked = linked_connection_.lock();
        if (!linked)
          throw std::system_error{make_error_code(std::errc::owner_dead)};

        async_write(linked->connection_.socket, buf,
          [self = shared_from_this()](const std::error_code& error,
            const std::size_t /*written_byte_count*/)
          {
            if (error) {
              self->finish(error);
              return;
            }

            self->async_wait_read_ready();
          });
      } else
        async_wait_read_ready();
    } catch (const boost::system::system_error& e) {
      finish(e.code());
    } catch (const std::system_error& e) {
      finish(e.code());
    } catch (...) {
      finish(make_error_code(std::errc::operation_canceled));
    }
  }

protected:
  /**
   * @brief Read-ready state handler.
   *
   * @details This function is called every time the connection()
   * state becomes read-ready.
   */
  virtual void handle_read_ready() = 0;

  /**
   * @brief Finish handler.
   *
   * @details This function is called every time the proxying is interrupted
   * by an `error`.
   */
  virtual void finish(const std::error_code& error) noexcept = 0;

private:
  Connection connection_;
  std::weak_ptr<Proxy_connection> linked_connection_;
};

} // namespace dmitigr::io

#endif  // DMITIGR_IO_CONNECTION_HPP
