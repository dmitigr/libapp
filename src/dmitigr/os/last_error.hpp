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

#ifndef DMITIGR_OS_LAST_ERROR_HPP
#define DMITIGR_OS_LAST_ERROR_HPP

#ifdef _WIN32
#include "windows.hpp"
#else
#include <cerrno>
#endif

namespace dmitigr::os {
/**
 * @returns The last system error code.
 *
 * @par Thread safety
 * Thread-safe.
 */
inline auto last_error() noexcept
{
#ifdef _WIN32
  // Note: the last-error code is maintained on a per-thread basis.
  return ::GetLastError();
#else
  /*
   * Note: errno is thread-local on Linux.
   * See also http://www.unix.org/whitepapers/reentrant.html
   */
  return errno;
#endif
}

} // namespace dmitigr::os

#endif  // DMITIGR_OS_LAST_ERROR_HPP
