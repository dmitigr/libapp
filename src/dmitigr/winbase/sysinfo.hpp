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

#pragma once
#pragma comment(lib, "kernel32")

#include "../base/assert.hpp"
#include "error.hpp"
#include "exceptions.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace dmitigr::winbase {

inline std::filesystem::path system_directory()
{
  std::wstring result;
  const auto size_with_null = GetSystemDirectoryW(result.data(), result.size());
  if (!size_with_null)
    throw Sys_exception{"cannot get system directory"};
  result.resize(size_with_null - 1);
  const auto sz = GetSystemDirectoryW(result.data(), result.size() + 1);
  if (!sz)
    throw Sys_exception{"cannot get system directory"};
  DMITIGR_ASSERT(sz == size_with_null - 1);
  return result;
}

inline std::wstring computer_name(const COMPUTER_NAME_FORMAT type)
{
  DWORD sz{};
  GetComputerNameExW(type, nullptr, &sz);
  if (const auto e = last_error(); e != ERROR_MORE_DATA)
    throw Sys_exception{e, "cannot get required size of computer name of type "
      +std::to_string(type)};
  std::wstring result(sz, L'\0');
  if (!GetComputerNameExW(type, result.data(), &sz))
    throw Sys_exception{"cannot get computer name of type "+std::to_string(type)};
  return result;
}

/**
 * @returns The string representation of a `value`.
 *
 * @remarks The `value` can be retrieved by calling GetSystemInfo().
 */
inline std::string cpu_architecture_string(const WORD value)
{
  switch (value) {
  case PROCESSOR_ARCHITECTURE_AMD64:
    return "x64";
  case PROCESSOR_ARCHITECTURE_ARM:
    return "arm";
  case PROCESSOR_ARCHITECTURE_ARM64:
    return "arm64";
  case PROCESSOR_ARCHITECTURE_IA64:
    return "ia64";
  case PROCESSOR_ARCHITECTURE_INTEL:
    return "x86";
  case PROCESSOR_ARCHITECTURE_UNKNOWN:
    [[fallthrough]];
  default:
    return "unknown";
  }
}

} // namespace dmitigr::winbase
