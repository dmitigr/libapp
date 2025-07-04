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

#ifndef DMITIGR_OS_PID_HPP
#define DMITIGR_OS_PID_HPP

#ifdef _WIN32
#include "windows.hpp"
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace dmitigr::os {

#ifdef _WIN32
/// The alias of the process identifier type.
using Pid = DWORD;
#else
/// The alias of the process identifier type.
using Pid = ::pid_t;
#endif

/// @returns The current process identifier of the calling process.
inline Pid pid() noexcept
{
#ifdef _WIN32
  return ::GetCurrentProcessId();
#else
  return ::getpid();
#endif
}

} // namespace dmitigr::os

#endif  // DMITIGR_OS_PID_HPP
