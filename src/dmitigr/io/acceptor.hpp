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

#ifndef DMITIGR_IO_ACCEPTOR_HPP
#define DMITIGR_IO_ACCEPTOR_HPP

#include "connection.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <system_error>
#include <utility>

namespace dmitigr::io {

/// An acceptor.
class Acceptor : public std::enable_shared_from_this<Acceptor> {
protected:
  struct Must_be_shared_ptr final {};

public:
  /// The destructor.
  virtual ~Acceptor() = default;

  /// The constructor.
  Acceptor(Must_be_shared_ptr, boost::asio::ip::tcp::acceptor acceptor)
    : acceptor_{std::move(acceptor)}
  {}

  /// Initiates to accept a connection.
  void async_accept()
  {
    try {
      acceptor_.async_accept(
        [self = shared_from_this()](const std::error_code& error,
          boost::asio::ip::tcp::socket peer)
        {
          if (error) {
            self->handle_error(error);
            return;
          }

          try {
            self->handle_accept(Connection{std::move(peer)});
          } catch (const boost::system::system_error& e) {
            self->handle_error(e.code());
          } catch (const std::system_error& e) {
            self->handle_error(e.code());
          } catch (...) {
            self->handle_error(make_error_code(std::errc::operation_canceled));
          }
        });
    } catch (const boost::system::system_error& e) {
      handle_error(e.code());
    } catch (const std::system_error& e) {
      handle_error(e.code());
    } catch (...) {
      handle_error(make_error_code(std::errc::operation_canceled));
    }
  }

protected:
  boost::asio::ip::tcp::acceptor acceptor_;

  /**
   * @brief Accept handler.
   *
   * @details This function is called every time the `connection` accepted.
   */
  virtual void handle_accept(Connection connection) = 0;

  /**
   * @brief Error handler.
   *
   * @details This function is called every time the accepting is interrupted
   * by an `error`.
   */
  virtual void handle_error(const std::error_code& error) noexcept = 0;
};

} // namespace dmitigr::io

#endif  // DMITIGR_IO_ACCEPTOR_HPP
