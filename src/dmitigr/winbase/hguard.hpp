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

/// A very thin wrapper around the HANDLE data type.
class Handle_guard final : private Noncopy {
public:
  /// The destructor.
  ~Handle_guard()
  {
    close();
  }

  /// The constructor.
  explicit Handle_guard(const HANDLE handle = INVALID_HANDLE_VALUE) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Handle_guard(Handle_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = INVALID_HANDLE_VALUE;
  }

  /// The move assignment operator.
  Handle_guard& operator=(Handle_guard&& rhs) noexcept
  {
    if (this != &rhs) {
      Handle_guard tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Handle_guard& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded HANDLE.
  HANDLE handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded HANDLE.
  operator HANDLE() const noexcept
  {
    return handle();
  }

  /// @returns `true` if the guarded HANDLE is valid.
  explicit operator bool() const noexcept
  {
    return handle_ != INVALID_HANDLE_VALUE;
  }

  /// @returns The error code.
  DWORD close() noexcept
  {
    DWORD result{ERROR_SUCCESS};
    if (handle_ != INVALID_HANDLE_VALUE) {
      if (CloseHandle(handle_))
        handle_ = INVALID_HANDLE_VALUE;
      else
        result = GetLastError();
    }
    return result;
  }

private:
  HANDLE handle_{INVALID_HANDLE_VALUE};
};

} // namespace dmitigr::winbase
