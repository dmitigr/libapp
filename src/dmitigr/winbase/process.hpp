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
#include "error.hpp"
#include "hguard.hpp"
#include "sync.hpp"
#include "windows.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <psapi.h>

namespace dmitigr::winbase {

/// A wrapper around PROCESS_INFORMATION.
class Process_info final : private Noncopy {
public:
  ~Process_info()
  {
    CloseHandle(info_.hThread);
    info_.hThread = INVALID_HANDLE_VALUE;
    CloseHandle(info_.hProcess);
    info_.hProcess = INVALID_HANDLE_VALUE;
  }

  Process_info() = default;

  Process_info(PROCESS_INFORMATION&& pi) noexcept
    : info_{std::move(pi)}
  {
    pi.hProcess = INVALID_HANDLE_VALUE;
    pi.hThread = INVALID_HANDLE_VALUE;
    pi.dwProcessId = -1;
    pi.dwThreadId = -1;
  }

  Process_info(Process_info&& rhs) noexcept
    : info_{std::move(rhs.info_)}
  {}

  Process_info& operator=(Process_info&& rhs) noexcept
  {
    Process_info tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Process_info& rhs) noexcept
  {
    using std::swap;
    swap(info_, rhs.info_);
  }

  const PROCESS_INFORMATION* ptr() const noexcept
  {
    return &info_;
  }

  PROCESS_INFORMATION* ptr() noexcept
  {
    return const_cast<PROCESS_INFORMATION*>(
      static_cast<const Process_info*>(this)->ptr());
  }

  const PROCESS_INFORMATION& ref() const noexcept
  {
    return info_;
  }

  PROCESS_INFORMATION& ref() noexcept
  {
    return const_cast<PROCESS_INFORMATION&>(
      static_cast<const Process_info*>(this)->ref());
  }

private:
  PROCESS_INFORMATION info_{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE,
      static_cast<DWORD>(-1), static_cast<DWORD>(-1)};
};

/// @returns A handle to an opened process.
inline Handle_guard open_process(const DWORD pid,
  const DWORD desired_access, const bool inherit_handle)
{
  HANDLE result{OpenProcess(desired_access, inherit_handle, pid)};
  if (result == NULL)
    throw std::runtime_error{last_error_message()};
  return Handle_guard{result};
}

/// @returns A token to an opened process.
inline Handle_guard open_process_token(const HANDLE process_handle,
  const DWORD desired_access)
{
  HANDLE result{INVALID_HANDLE_VALUE};
  if (!OpenProcessToken(process_handle, desired_access, &result))
    throw std::runtime_error{last_error_message()};
  return Handle_guard{result};
}

/// @returns The fully qualified path for the file that contains the specified `module`.
inline std::filesystem::path module_filename(const HMODULE module = {})
{
  constexpr const std::size_t sz_inc{259};
  std::wstring result(sz_inc, 0);
  while (true) {
    const DWORD result_size{GetModuleFileNameW(module,
      result.data(), result.size())};
    const DWORD err{GetLastError()};
    if (err == ERROR_INSUFFICIENT_BUFFER) {
      result.resize(result.size() + sz_inc);
    } else if (!result_size) {
      throw std::runtime_error{system_message(err)};
    } else if (result_size <= result.size()) {
      result.resize(result_size);
      break;
    } else
      throw std::logic_error{"bug of GetModuleFileNameW()"};
  }
  return result;
}

/**
 * @returns The full name of the executable image for the specified `process`.
 *
 * @param flags A value of `0` means Win32 path format.
 */
inline std::wstring query_full_process_image_name(const HANDLE process,
  const DWORD flags = 0)
{
  constexpr const std::size_t sz_inc{259};
  std::wstring result(sz_inc, 0);
  while (true) {
    auto sz = static_cast<DWORD>(result.size() + 1);
    if (!QueryFullProcessImageNameW(process, flags, result.data(), &sz)) {
      if (const DWORD err{GetLastError()}; err == ERROR_INSUFFICIENT_BUFFER)
        result.resize(result.size() + sz_inc);
      else
        throw std::runtime_error{system_message(err)};
    } else {
      result.resize(sz);
      break;
    }
  }
  return result;
}

/// @overload
inline std::wstring query_full_process_image_name(const DWORD process_id,
  const DWORD flags = 0)
{
  const auto process = open_process(process_id, PROCESS_QUERY_LIMITED_INFORMATION, false);
  return query_full_process_image_name(process, flags);
}

/// @returns A vector of PIDs of process objects in the system.
inline std::vector<DWORD> enum_processes()
{
  constexpr const std::size_t sz_inc{512};
  std::vector<DWORD> result(sz_inc, -1);
  while (true) {
    using Pid = decltype(result)::value_type;
    const auto result_size_in_bytes = static_cast<DWORD>(result.size()*sizeof(Pid));
    DWORD needed_sz{};
    if (!EnumProcesses(result.data(), result_size_in_bytes, &needed_sz))
      throw std::runtime_error{last_error_message()};
    else if (needed_sz < result_size_in_bytes) {
      result.resize(needed_sz / sizeof(Pid));
      break;
    } else
      result.resize(result.size() + sz_inc);
  }
  return result;
}

/// @returns The termination status of the specified process.
inline DWORD exit_code_process(const HANDLE handle)
{
  DWORD result{};
  if (!GetExitCodeProcess(handle, &result))
    throw std::runtime_error{last_error_message()};
  return result;
}

/// @returns The termination status of the specified process which terminated.
inline DWORD wait_for_exit(const HANDLE process,
  const std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
{
  const auto status = wait_for_single_object(process, timeout);
  if (status == WAIT_TIMEOUT)
    throw std::runtime_error{"process wait timeout"};
  else if (status != WAIT_OBJECT_0)
    throw std::runtime_error{"process wait error "+std::to_string(status)};
  return exit_code_process(process);
}

} // namespace dmitigr::winbase
