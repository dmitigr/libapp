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

#ifndef DMITIGR_WINBASE_EXCEPTIONS_HPP
#define DMITIGR_WINBASE_EXCEPTIONS_HPP

#include "windows.hpp"

#include <string>
#include <system_error>

namespace dmitigr::winbase {

enum Throw_modifier {
  throw_all,
  no_throw_if_not_found
};

/**
 * @ingroup errors
 *
 * @brief An exception thrown on system error.
 *
 * @details The system category includes:
 *   - Network Manager Errors in range [NERR_BASE, MAX_NERR].
 */
class Sys_exception final : public std::system_error {
public:
  /// The constructor.
  Sys_exception(const DWORD ev, const std::string& what)
    : system_error{static_cast<int>(ev), std::system_category(), what}
  {}

  /// @overload
  explicit Sys_exception(const std::string& what)
    : Sys_exception{GetLastError(), what}
  {}
};

} // namespace dmitigr::winbase

#endif  // DMITIGR_WINBASE_EXCEPTIONS_HPP
