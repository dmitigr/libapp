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

#include "exceptions.hpp"
#include "hguard.hpp"

#include <chrono>

namespace dmitigr::winbase {

inline Handle_guard create_event(const LPSECURITY_ATTRIBUTES secattrs,
  const bool manual_reset, const bool initial_state, LPCWSTR name = {})
{
  auto result = CreateEventW(secattrs, manual_reset, initial_state, name);
  if (!result)
    throw Sys_exception{"cannot create event"};
  return Handle_guard{result};
}

inline Handle_guard open_event(const DWORD desired_access,
  const bool inherit, LPCWSTR name)
{
  auto result = OpenEventW(desired_access, inherit, name);
  if (!result)
    throw Sys_exception{"cannot open event"};
  return Handle_guard{result};
}

inline void set_event(const HANDLE hdl)
{
  if (!SetEvent(hdl))
    throw Sys_exception{"cannot set event"};
}

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
    throw Sys_exception{"cannot wait for single object"};
  return result;
}

} // namespace dmitigr::winbase
