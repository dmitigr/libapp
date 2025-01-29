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

#include "../base/assert.hpp"
#include "../base/noncopymove.hpp"
#include "exceptions.hpp"
#include "windows.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace dmitigr::winbase {

// -----------------------------------------------------------------------------
// Account
// -----------------------------------------------------------------------------

class Account final {
public:
  explicit Account(const PSID sid)
    : Account{sid, std::wstring{}}
  {}

  explicit Account(std::wstring name)
    : Account{std::move(name), std::wstring{}}
  {}

  Account(const PSID sid, const std::wstring& system_name)
  {
    DWORD name_size{};
    DWORD domain_size{};
    const LPCWSTR system{!system_name.empty() ? system_name.c_str() : nullptr};

    LookupAccountSidW(system, sid,
      nullptr, &name_size, nullptr, &domain_size, nullptr);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      throw_cannot_create();

    name_.resize(name_size - 1);
    domain_.resize(domain_size - 1);
    if (!LookupAccountSidW(system, sid,
        name_.data(), &name_size,
        domain_.data(), &domain_size, &type_))
      throw_cannot_create();

    DMITIGR_ASSERT(IsValidSid(sid));
    sid_buf_.resize(GetLengthSid(sid));
    if (!CopySid(static_cast<DWORD>(sid_buf_.size()), sid_buf_.data(), sid))
      throw_cannot_create();
  }

  Account(std::wstring name, const std::wstring& system_name)
    : name_{std::move(name)}
  {
    const LPCWSTR system{!system_name.empty() ? system_name.c_str() : nullptr};
    DWORD sid_buf_size{};
    DWORD domain_size{};
    LookupAccountNameW(system, name_.c_str(), nullptr, &sid_buf_size,
      nullptr, &domain_size, nullptr);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      throw_cannot_create();

    sid_buf_.resize(sid_buf_size);
    domain_.resize(domain_size - 1);
    if (!LookupAccountNameW(system, name_.c_str(),
        sid_buf_.data(), &sid_buf_size,
        domain_.data(), &domain_size, &type_))
      throw_cannot_create();
  }

  void swap(Account& rhs) noexcept
  {
    using std::swap;
    swap(type_, rhs.type_);
    swap(sid_buf_, rhs.sid_buf_);
    swap(name_, rhs.name_);
    swap(domain_, rhs.domain_);
  }

  SID_NAME_USE type() const noexcept
  {
    return type_;
  }

  const PSID sid() const noexcept
  {
    return !sid_buf_.empty() ?
      reinterpret_cast<PSID>(const_cast<char*>(sid_buf_.data())) : nullptr;
  }

  PSID sid() noexcept
  {
    return const_cast<PSID>(static_cast<const Account*>(this)->sid());
  }

  const std::wstring& name() const noexcept
  {
    return name_;
  }

  const std::wstring& domain() const noexcept
  {
    return domain_;
  }

private:
  SID_NAME_USE type_{};
  std::vector<char> sid_buf_;
  std::wstring name_;
  std::wstring domain_;

  [[noreturn]] void throw_cannot_create() const
  {
    throw Sys_exception{"cannot create Account instance"};
  }
};

} // namespace dmitigr::winbase
