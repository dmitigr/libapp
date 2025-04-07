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

// The Windows System Services

#pragma once
#pragma comment(lib, "advapi32")

#include "../base/noncopymove.hpp"
#include "exceptions.hpp"
#include "windows.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace dmitigr::winbase::service {

// -----------------------------------------------------------------------------
// Sc_handle
// -----------------------------------------------------------------------------

/// A SC_HANDLE RAII-style guard.
class Sc_handle final : private Noncopy {
public:
  /// The destructor.
  ~Sc_handle()
  {
    close();
  }

  /// The default constructor.
  Sc_handle() noexcept = default;

  /// The constructor.
  explicit Sc_handle(const SC_HANDLE handle) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Sc_handle(Sc_handle&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = NULL;
  }

  /// The move assignment operator.
  Sc_handle& operator=(Sc_handle&& rhs) noexcept
  {
    if (this != &rhs) {
      Sc_handle tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Sc_handle& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded SC_HANDLE.
  SC_HANDLE handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded SC_HANDLE.
  operator SC_HANDLE() const noexcept
  {
    return handle();
  }

  /// @returns `true` if the guarded SC_HANDLE is valid.
  explicit operator bool() const noexcept
  {
    return handle_ != NULL;
  }

  /// Closes handle.
  DWORD close() noexcept
  {
    DWORD result{ERROR_SUCCESS};
    if (handle_ != NULL) {
      if (CloseServiceHandle(handle_))
        handle_ = NULL;
      else
        result = GetLastError();
    }
    return result;
  }

  /// @returns The released handle.
  SC_HANDLE release() noexcept
  {
    auto result = handle_;
    handle_ = NULL;
    return result;
  }

private:
  SC_HANDLE handle_{NULL};
};

// -----------------------------------------------------------------------------
// Service_config
// -----------------------------------------------------------------------------

class Service_config final {
public:
  Service_config() = default;

  explicit Service_config(const SC_HANDLE service)
  {
    DWORD buf_size{};
    QueryServiceConfigW(service, nullptr, buf_size, &buf_size);
    if (!buf_size)
      throw Sys_exception{"cannot determine buffer size to store all the"
        " service configuration information"};

    data_.resize(buf_size);
    auto* const qsc = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(data_.data());
    if (!QueryServiceConfigW(service, qsc, buf_size, &buf_size))
      throw Sys_exception{"cannot query service configuration information"};
  }

  const QUERY_SERVICE_CONFIGW* ptr() const noexcept
  {
    return !data_.empty() ?
      reinterpret_cast<const QUERY_SERVICE_CONFIGW*>(data_.data()) : nullptr;
  }

private:
  std::vector<char> data_;
};

inline SERVICE_DELAYED_AUTO_START_INFO
query_service_delayed_auto_start_info(const SC_HANDLE service)
{
  SERVICE_DELAYED_AUTO_START_INFO result{};
  DWORD required_size{};
  if (!QueryServiceConfig2W(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
      reinterpret_cast<LPBYTE>(&result), sizeof(result), &required_size))
    throw Sys_exception{"cannot query service delayed auto start info"};
  return result;
}

inline void change_service_delayed_auto_start_info(const SC_HANDLE service,
  SERVICE_DELAYED_AUTO_START_INFO info)
{
  if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
      &info))
    throw Sys_exception{"cannot change service delayed auto start info"};
}

// -----------------------------------------------------------------------------

inline Sc_handle open_manager(const DWORD desired_access,
  const LPCWSTR machine_name = {}, const LPCWSTR database_name = {})
{
  if (const auto h = OpenSCManagerW(machine_name, database_name, desired_access))
    return Sc_handle{h};
  else
    throw Sys_exception{"cannot open service manager"};
}

inline Sc_handle open_service(const SC_HANDLE manager,
  const LPCWSTR service_name, const DWORD desired_access)
{
  if (const auto h = OpenServiceW(manager, service_name, desired_access); h != NULL)
    return Sc_handle{h};
  else
    throw Sys_exception{"cannot open service"};
}

inline SERVICE_STATUS_PROCESS
query_service_status_process_info(const SC_HANDLE service)
{
  SERVICE_STATUS_PROCESS ssp{};
  DWORD required_size{};
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
      reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &required_size))
    throw Sys_exception{"cannot query service status process info"};
  return ssp;
}

// -----------------------------------------------------------------------------

inline void change_service_config(const SC_HANDLE service,
  const QUERY_SERVICE_CONFIGW& config,
  LPDWORD tag_id = {}, LPCWSTR const password = {})
{
  if (!ChangeServiceConfigW(service, config.dwServiceType, config.dwStartType,
    config.dwErrorControl, config.lpBinaryPathName, config.lpLoadOrderGroup,
    tag_id, config.lpDependencies, config.lpServiceStartName, password,
      config.lpDisplayName))
    throw Sys_exception{"cannot change service config"};
}

} // namespace dmitigr::winbase::service
