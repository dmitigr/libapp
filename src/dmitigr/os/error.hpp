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

#ifndef DMITIGR_OS_ERROR_HPP
#define DMITIGR_OS_ERROR_HPP

#ifndef _WIN32
#include "../nix/error.hpp"
#endif

#include "../base/assert.hpp"
#include "exceptions.hpp"
#include "last_error.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace dmitigr::os {

/// @returns String describing OS error code.
inline std::string error_message(const int code)
{
#ifdef _WIN32
  char buf[128];
  if (const int e = ::strerror_s(buf, sizeof(buf), code))
    throw Sys_exception{e, "cannot get an OS error message"};
  else
    return buf;
#else
  return nix::error_message(code);
#endif
}

/// Prints the last system error to the standard error.
inline void print_last_error(const char* const context) noexcept
{
  DMITIGR_ASSERT(context);
  std::fprintf(stderr, "%s: error %d\n", context, last_error());
}

} // namespace dmitigr::os

#endif  // DMITIGR_OS_ERROR_HPP
