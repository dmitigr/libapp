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

#ifndef DMITIGR_IO_ASYNC_AGENT_HPP
#define DMITIGR_IO_ASYNC_AGENT_HPP

#include <boost/asio.hpp>

#include <memory>
#include <system_error>

namespace dmitigr::io {

class Async_agent : public std::enable_shared_from_this<Async_agent> {
public:
  /// The destructor.
  virtual ~Async_agent() = default;

protected:
  /// A tag to allow creation of instance only via special function.
  struct Must_be_shared_ptr final {};

  /**
   * @brief Error handler.
   *
   * @details This function is called every time the error occurred upon of
   * either asynchronous operation initiation or asynchronous operation continuation.
   */
  virtual void handle_error(const std::error_code& error) noexcept = 0;

  /// Calls `callback` within the try-block to call handle_error() on exception.
  template<typename F>
  decltype(auto) with_handle_error(F&& callback) noexcept
  {
    try {
      return callback();
    } catch (const boost::system::system_error& e) {
      handle_error(e.code());
    } catch (const std::system_error& e) {
      handle_error(e.code());
    } catch (...) {
      handle_error(make_error_code(std::errc::operation_canceled));
    }
  }
};

} // namespace dmitigr::io

#endif  // DMITIGR_IO_ASYNC_AGENT_HPP
