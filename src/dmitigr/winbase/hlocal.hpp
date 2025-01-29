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

/// A very thin wrapper around the HLOCAL data type.
class Hlocal_guard final : private Noncopy {
public:
  /// The destructor.
  ~Hlocal_guard()
  {
    close();
  }

  /// The constructor.
  explicit Hlocal_guard(const HLOCAL handle = NULL) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Hlocal_guard(Hlocal_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = NULL;
  }

  /// The move assignment operator.
  Hlocal_guard& operator=(Hlocal_guard&& rhs) noexcept
  {
    if (this != &rhs) {
      Hlocal_guard tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Hlocal_guard& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded HLOCAL.
  HLOCAL handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded HLOCAL.
  operator HLOCAL() const noexcept
  {
    return handle();
  }

  /// @returns The error code.
  DWORD close() noexcept
  {
    DWORD result{ERROR_SUCCESS};
    if (handle_ != NULL) {
      if (!LocalFree(handle_))
        handle_ = NULL;
      else
        result = GetLastError();
    }
    return result;
  }

private:
  HLOCAL handle_{NULL};
};

} // namespace dmitigr::winbase
