// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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

// Windows on Windows 64

#pragma once
#pragma comment(lib, "kernel32")

#include "exceptions.hpp"
#include "windows.hpp"

#include <wow64apiset.h>

namespace dmitigr::winbase::wow64 {

inline int bitness()
{
#if defined(_WIN64)
  return 64;
#elif defined(_WIN32) && (_WIN32_WINNT >= 0x0A00)
  USHORT process_machine{};
  constexpr USHORT* const native_machine{nullptr};
  const auto e = IsWow64Process2(GetCurrentProcess(),
    &process_machine, nullptr);
  if (e)
    throw Sys_exception{"cannot detemine Windows bitness"};
  return process_machine == IMAGE_FILE_MACHINE_UNKNOWN ? 32 : 64;
#else
#error Unsupported Windows version.
#endif
}

} // namespace dmitigr::winbase::wow64
