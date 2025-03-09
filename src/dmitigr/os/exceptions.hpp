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

#ifndef DMITIGR_OS_EXCEPTIONS_HPP
#define DMITIGR_OS_EXCEPTIONS_HPP

#include "../base/exceptions.hpp"
#include "last_error.hpp"

#include <string>
#include <system_error>

namespace dmitigr::os {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public dmitigr::Exception {
  using dmitigr::Exception::Exception;
};

// -----------------------------------------------------------------------------
// Sys_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception thrown on system error.
 */
class Sys_exception final : public Exception {
public:
  /// The constructor.
  explicit Sys_exception(const std::string& what)
    : Sys_exception{static_cast<int>(last_error()), what}
  {}

  /// @overload
  Sys_exception(const int ev, const std::string& what)
    : Exception{std::error_code{ev, std::system_category()}, what}
  {}
};

} // namespace dmitigr::os

#endif  // DMITIGR_OS_EXCEPTIONS_HPP
