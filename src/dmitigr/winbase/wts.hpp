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

// Windows Terminal Services

#pragma once
#pragma comment(lib, "wtsapi32")

#include "../base/noncopymove.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <wtsapi32.h>

namespace dmitigr::winbase::wts {

constexpr const DWORD invalid_session_id{0xFFFFFFFF};

class Session_info_by_class final : private Noncopy {
public:
  ~Session_info_by_class()
  {
    if (data_)
      WTSFreeMemory(data_);
  }

  Session_info_by_class() = default;

  /**
   * Constructs the instance for session information `info_class` for the
   * specified `session_id` on the specified Remote Desktop Session
   * Host `server`.
   */
  Session_info_by_class(const HANDLE server, const DWORD session_id,
    const WTS_INFO_CLASS info_class)
    : info_class_{info_class}
  {
    if (!WTSQuerySessionInformationW(server, session_id, info_class,
        &data_, &size_in_bytes_))
      throw Sys_exception{"cannot query RDP session information"};
  }

  Session_info_by_class(Session_info_by_class&& rhs) noexcept
    : data_{rhs.data_}
    , size_in_bytes_{rhs.size_in_bytes_}
    , info_class_{rhs.info_class_}
  {
    rhs.data_ = {};
    rhs.size_in_bytes_ = {};
    rhs.info_class_ = {};
  }

  Session_info_by_class& operator=(Session_info_by_class&& rhs) noexcept
  {
    Session_info_by_class tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Session_info_by_class& rhs) noexcept
  {
    using std::swap;
    swap(data_, rhs.data_);
    swap(size_in_bytes_, rhs.size_in_bytes_);
    swap(info_class_, rhs.info_class_);
  }

  WTS_INFO_CLASS info_class() const noexcept
  {
    return info_class_;
  }

  LPCWSTR data() const noexcept
  {
    return data_;
  }

  std::size_t size_in_bytes() const noexcept
  {
    return size_in_bytes_;
  }

  template<typename T>
  T to() const
  {
    using std::is_same_v;
    using D = std::decay_t<T>;
    if constexpr (is_same_v<D, std::wstring_view> || is_same_v<D, std::wstring>) {
      if (!size_in_bytes_ || (size_in_bytes_ % sizeof(wchar_t)))
        throw std::runtime_error{"cannot convert RDP session information to wide"
          " string"};
      return D{data_, size_in_bytes_/sizeof(wchar_t) - 1};
    } else {
      D result;
      if (!(size_in_bytes_ == sizeof(T)))
        throw std::runtime_error{"cannot convert RDP session information to T"};
      std::memcpy(std::addressof(result), data_, size_in_bytes_);
      return result;
    }
  }

private:
  LPWSTR data_{};
  DWORD size_in_bytes_{};
  WTS_INFO_CLASS info_class_{};
};

// -----------------------------------------------------------------------------

class Session_enumeration final : private Noncopy {
public:
  ~Session_enumeration()
  {
    if (info_)
      WTSFreeMemoryExW(WTSTypeSessionInfoLevel1, info_, info_count_);
  }

  Session_enumeration() = default;

  /// Constructs enumeration of sessions on a Remote Desktop Session Host `server`.
  explicit Session_enumeration(const HANDLE server)
  {
    DWORD level{1};
    if (!WTSEnumerateSessionsExW(server, &level, 0, &info_, &info_count_))
      throw Sys_exception{"cannot enumerate server sessions"};
  }

  Session_enumeration(Session_enumeration&& rhs) noexcept
    : info_{rhs.info_}
    , info_count_{rhs.info_count_}
  {
    rhs.info_ = {};
    rhs.info_count_ = {};
  }

  Session_enumeration& operator=(Session_enumeration&& rhs) noexcept
  {
    Session_enumeration tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Session_enumeration& rhs) noexcept
  {
    using std::swap;
    swap(info_, rhs.info_);
    swap(info_count_, rhs.info_count_);
  }

  const PWTS_SESSION_INFO_1W info() const noexcept
  {
    return info_;
  }

  DWORD count() const noexcept
  {
    return info_count_;
  }

private:
  PWTS_SESSION_INFO_1W info_{};
  DWORD info_count_{};
};

} // namespace dmitigr::winbase::wts
