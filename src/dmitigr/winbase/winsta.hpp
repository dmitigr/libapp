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

#include "../base/noncopymove.hpp"
#include "windows.hpp"

#include <algorithm>
#include <utility>

namespace dmitigr::winbase {

/// A very thin wrapper around the HDESK data type.
class Hdesk_guard final : private Noncopy {
public:
  /// The destructor.
  ~Hdesk_guard()
  {
    close();
  }

  /// The constructor.
  explicit Hdesk_guard(const HDESK handle = NULL) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Hdesk_guard(Hdesk_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = NULL;
  }

  /// The move assignment operator.
  Hdesk_guard& operator=(Hdesk_guard&& rhs) noexcept
  {
    if (this != &rhs) {
      Hdesk_guard tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Hdesk_guard& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded HDESK.
  HDESK handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded HDESK.
  operator HDESK() const noexcept
  {
    return handle();
  }

  /// @returns The error code.
  DWORD close() noexcept
  {
    DWORD result{ERROR_SUCCESS};
    if (handle_ != NULL) {
      if (CloseDesktop(handle_))
        handle_ = NULL;
      else
        result = GetLastError();
    }
    return result;
  }

private:
  HDESK handle_{NULL};
};

/// A very thin wrapper around the HWINSTA data type.
class Hwinsta_guard final : private Noncopy {
public:
  /// The destructor.
  ~Hwinsta_guard()
  {
    close();
  }

  /// The constructor.
  explicit Hwinsta_guard(const HWINSTA handle = NULL) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Hwinsta_guard(Hwinsta_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = NULL;
  }

  /// The move assignment operator.
  Hwinsta_guard& operator=(Hwinsta_guard&& rhs) noexcept
  {
    if (this != &rhs) {
      Hwinsta_guard tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Hwinsta_guard& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded HWINSTA.
  HWINSTA handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded HWINSTA.
  operator HWINSTA() const noexcept
  {
    return handle();
  }

  /// @returns The error code.
  DWORD close() noexcept
  {
    DWORD result{ERROR_SUCCESS};
    if (handle_ != NULL) {
      if (CloseWindowStation(handle_))
        handle_ = NULL;
      else
        result = GetLastError();
    }
    return result;
  }

private:
  HWINSTA handle_{NULL};
};

} // namespace dmitigr::winbase
