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
#pragma comment(lib, "kernel32")

#include "error.hpp"

#include <chrono>
#include <stdexcept>

namespace dmitigr::winbase {

/**
 * @param handle A handle to the object.
 * @param timeout A timeout interval. The value of `timeout.max()` denotes INFINITE.
 */
inline DWORD wait_for_single_object(const HANDLE handle,
  const std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
{
  using Ms = std::chrono::milliseconds;
  const DWORD timeout_native{
    timeout == timeout.max() ? INFINITE : static_cast<DWORD>(timeout.count())};
  const auto result = WaitForSingleObject(handle, timeout_native);
  if (result == WAIT_FAILED)
    throw std::runtime_error{last_error_message()};
  return result;
}

} // namespace dmitigr::winbase
