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

#include "async_agent.hpp"
#include "connection.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <system_error>
#include <utility>

namespace dmitigr::io {

/// An acceptor.
class Acceptor : public Async_agent {
public:
  /// The constructor.
  Acceptor(Must_be_shared_ptr, boost::asio::ip::tcp::acceptor acceptor)
    : acceptor_{std::move(acceptor)}
  {}

  /// Initiates connection acceptance.
  void async_accept() noexcept
  {
    with_handle_error([this]
    {
      acceptor_.async_accept(
        [self = std::static_pointer_cast<Acceptor>(shared_from_this())]
        (const std::error_code& error, boost::asio::ip::tcp::socket peer)
        {
          if (error)
            return self->handle_error(error, "async acceptation error");

          self->with_handle_error([&self, &peer]
          {
            self->handle_accept(Connection{std::move(peer)});
          });
        });
    });
  }

protected:
  boost::asio::ip::tcp::acceptor acceptor_;

  /**
   * @brief Accept handler.
   *
   * @details This function is called every time the `connection` accepted.
   */
  virtual void handle_accept(Connection connection) = 0;
};

} // namespace dmitigr::io

#endif  // DMITIGR_IO_ACCEPTOR_HPP
