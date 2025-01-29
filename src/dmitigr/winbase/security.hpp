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
#pragma comment(lib, "advapi32")

#include "exceptions.hpp"
#include "windows.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace dmitigr::winbase {

// -----------------------------------------------------------------------------
// Sid
// -----------------------------------------------------------------------------

class Sid final : private Noncopy {
public:
  ~Sid()
  {
    if (ptr_)
      FreeSid(ptr_);
  }

  Sid() = default;

  Sid(Sid&& rhs) noexcept
    : ptr_{rhs.ptr_}
  {
    rhs.ptr_ = {};
  }

  Sid& operator=(Sid&& rhs) noexcept
  {
    Sid tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Sid& rhs) noexcept
  {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }

  template<typename ... S>
  Sid(SID_IDENTIFIER_AUTHORITY authority, const S ... sub_authorities)
  {
    constexpr const auto sub_auth_sz = sizeof...(sub_authorities);
    static_assert(sub_auth_sz <= 8);
    std::tuple<PSID_IDENTIFIER_AUTHORITY, BYTE,
      DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID*> args{};
    std::get<0>(args) = &authority;
    std::get<1>(args) = static_cast<BYTE>(sub_auth_sz);
    fill_subs(args, std::make_index_sequence<sub_auth_sz>{},
      std::forward<const DWORD>(sub_authorities)...);
    std::get<10>(args) = &ptr_;
    if (!std::apply(AllocateAndInitializeSid, args))
      throw Sys_exception{"cannot create Sid instance"};
  }

  const PSID ptr() const noexcept
  {
    return ptr_;
  }

private:
  PSID ptr_{};

  template<class Tuple, std::size_t ... I, typename ... S>
  void fill_subs(Tuple& args, std::index_sequence<I...> seq,
    const S ... sub_authorities)
  {
    static_assert(sizeof...(sub_authorities) == seq.size());
    ((std::get<I + 2>(args) = sub_authorities), ...);
  }
};

class Token_info final {
public:
  Token_info() = default;

  Token_info(const HANDLE token, const TOKEN_INFORMATION_CLASS type)
  {
    reset(token, type);
  }

  void reset(const HANDLE token, const TOKEN_INFORMATION_CLASS type)
  {
    constexpr const char* const errmsg{"cannot reset Token_info"};
    DWORD sz{};
    GetTokenInformation(token, type, nullptr, 0, &sz);
    if (!(sz > 0)) {
      if (const auto err = GetLastError())
        throw Sys_exception{err, errmsg};
    }

    buf_.resize(sz);
    if (!GetTokenInformation(token, type, buf_.data(),
        static_cast<DWORD>(buf_.size()), &sz))
      throw Sys_exception{errmsg};

    type_ = type;
  }

  TOKEN_INFORMATION_CLASS type() const noexcept
  {
    return type_;
  }

  template<class T>
  const T& data() const noexcept
  {
    return *reinterpret_cast<const T*>(buf_.data());
  }

  template<class T>
  T& data() noexcept
  {
    return const_cast<T&>(static_cast<const Token_info*>(this)->data<T>());
  }

  const void* bytes() const noexcept
  {
    return buf_.data();
  }

  void* bytes() noexcept
  {
    return const_cast<void*>(static_cast<const Token_info*>(this)->bytes());
  }

  DWORD size() const noexcept
  {
    return static_cast<DWORD>(buf_.size());
  }

private:
  TOKEN_INFORMATION_CLASS type_{};
  std::vector<char> buf_;
};

// -----------------------------------------------------------------------------

/**
 * @returns Locally unique identifier (LUID) used on a specified system to
 * locally represent the specified privilege name.
 */
inline LUID lookup_privilege_value(const std::wstring& privilege_name,
  const std::wstring& system_name = {})
{
  LUID result{};
  if (!LookupPrivilegeValueW(
      system_name.empty() ? nullptr : system_name.c_str(),
      privilege_name.c_str(), &result))
    throw Sys_exception{"cannot lookup privilege value"};
  return result;
}

/**
 * @returns The name that corresponds to the privilege represented on a specific
 * system by a specified locally unique identifier (LUID).
 */
inline std::wstring lookup_privilege_name(LUID luid,
  const std::wstring& system_name = {})
{
  DWORD sz{64 + 1};
  std::wstring result(sz - 1, 0);
  while (true) {
    if (!LookupPrivilegeNameW(
        system_name.empty() ? nullptr : system_name.c_str(),
        &luid, result.data(), &sz)) {
      if (const DWORD err{GetLastError()}; err == ERROR_INSUFFICIENT_BUFFER)
        result.resize(sz - 1);
      else
        throw Sys_exception{err, "cannot lookup privilege name"};
    } else {
      result.resize(sz);
      break;
    }
  }
  return result;
}

/// A token privileges.
class Token_privileges final {
public:
  Token_privileges()
    : Token_privileges{0}
  {}

  explicit Token_privileges(const DWORD count)
    : data_(required_data_size(count))
  {
    data()->PrivilegeCount = count;
  }

  void resize(const DWORD count)
  {
    data_.resize(required_data_size(count));
    data()->PrivilegeCount = count;
  }

  DWORD size() const noexcept
  {
    return data()->PrivilegeCount;
  }

  DWORD size_in_bytes() const noexcept
  {
    return static_cast<DWORD>(data_.size());
  }

  void set(const DWORD index, const LUID luid, const DWORD attributes)
  {
    if (!(0 <= index && index < size()))
      throw std::invalid_argument{"invalid privilege index"};

    data()->Privileges[index].Luid = luid;
    data()->Privileges[index].Attributes = attributes;
  }

  void set(const DWORD index, const std::wstring& privilege_name,
    const std::wstring& system_name, const DWORD attributes)
  {
    set(index, lookup_privilege_value(privilege_name, system_name), attributes);
  }

  void set(const DWORD index, const std::wstring& privilege_name,
    const DWORD attributes)
  {
    set(index, privilege_name, std::wstring{}, attributes);
  }

  const TOKEN_PRIVILEGES* data() const noexcept
  {
    return reinterpret_cast<const TOKEN_PRIVILEGES*>(data_.data());
  }

  TOKEN_PRIVILEGES* data() noexcept
  {
    return const_cast<TOKEN_PRIVILEGES*>(
      static_cast<const Token_privileges*>(this)->data());
  }

private:
  std::vector<char> data_;

  static std::size_t required_data_size(const DWORD count)
  {
    if (count < 0)
      throw std::invalid_argument{"invalid privilege count"};
    return sizeof(TOKEN_PRIVILEGES::PrivilegeCount) +
      sizeof(TOKEN_PRIVILEGES::Privileges) * count;
  }
};

/**
 * @brief Toggles privileges in the specified access `token`.
 *
 * @returns A pair of token privileges and error code, which can be
 *   - `ERROR_SUCCESS` indicating that the function adjusted all
 *   specified privileges;
 *   - `ERROR_NOT_ALL_ASSIGNED` indicating the `token` doesn't have
 *   one or more of the privileges specified in the `new_state`.
 *
 * @param token An access token.
 * @param disable_all_privileges If `true`, the function disables all privileges
 * ignoring the `new_state` parameter.
 * @param new_state Privileges for the `token` to be enabled, disabled or removed.
 *
 * @par Requires
 * `token` requires `TOKEN_ADJUST_PRIVILEGES` access.
 */
inline std::pair<Token_privileges, DWORD>
adjust_token_privileges(const HANDLE token,
  const bool disable_all_privileges,
  const Token_privileges& new_state)
{
  auto prev_state = new_state;
  auto prev_state_size_in_bytes = prev_state.size_in_bytes();
  if (!AdjustTokenPrivileges(token, disable_all_privileges,
      const_cast<TOKEN_PRIVILEGES*>(new_state.data()), new_state.size_in_bytes(),
      prev_state.data(), &prev_state_size_in_bytes))
    throw Sys_exception{"cannot adjust token privileges"};
  prev_state.resize(prev_state.data()->PrivilegeCount);
  assert(prev_state.size_in_bytes() <= prev_state_size_in_bytes);
  return std::make_pair(std::move(prev_state), GetLastError());
}

/// Sets an `info` for a specified access `token`.
inline void set_token_information(const HANDLE token, const Token_info& info)
{
  if (!SetTokenInformation(token, info.type(),
      const_cast<void*>(info.bytes()), info.size()))
    throw Sys_exception{"cannot set token information"};
}

/// @overload
inline void set_token_information(const HANDLE token,
  const TOKEN_INFORMATION_CLASS type, DWORD value)
{
  if (!SetTokenInformation(token, type, &value, sizeof(value)))
    throw Sys_exception{"cannot set token information"};
}

} // namespace dmitigr::winbase
