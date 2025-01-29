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

#pragma once

#include "windows.hpp"
#include "hlocal.hpp"
#include "strconv.hpp"

#include <stdexcept>
#include <string>

namespace dmitigr::winbase {

/// @returns The calling thread's last-error code value.
inline DWORD last_error() noexcept
{
  return GetLastError();
}

/// @returns A human-readable system message in UTF-16.
inline std::wstring system_message_w(const DWORD message_id)
{
  LPWSTR buffer{};
  const auto buffer_size = FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS |
    FORMAT_MESSAGE_MAX_WIDTH_MASK,
    nullptr,
    message_id,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    // MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
    reinterpret_cast<LPWSTR>(&buffer),
    0,
    nullptr);
  if (!buffer_size) {
    throw std::runtime_error{"cannot create system error message:"
      " error " + std::to_string(GetLastError())};
  }

  const Hlocal_guard guard{buffer};
  return std::wstring{buffer, buffer_size};
}

/// @returns A human-readable system message in UTF-16.
inline std::string system_message(const DWORD message_id)
{
  return utf16_to_utf8(system_message_w(message_id));
}

/// @returns A human-readable message from GetLastError() in UTF-16.
inline std::wstring last_error_message_w()
{
  return system_message_w(GetLastError());
}

/// @returns A human-readable message from GetLastError() in UTF-8.
inline std::string last_error_message()
{
  return utf16_to_utf8(last_error_message_w());
}

} // namespace dmitigr::winbase
