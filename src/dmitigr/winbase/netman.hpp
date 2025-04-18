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

// Network Management

#pragma once
#pragma comment(lib, "netapi32")

#include "../base/traits.hpp"
#include "exceptions.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <Lm.h>

namespace dmitigr::winbase::netman {

namespace detail {

inline std::vector<LOCALGROUP_MEMBERS_INFO_0> to_vector_lgmi0(
  const std::vector<PSID>& members)
{
  std::vector<LOCALGROUP_MEMBERS_INFO_0> result;
  result.reserve(members.size());
  for (const auto psid : members)
    result.emplace_back(psid);
  return result;
}

} // namespace detail

template<class T>
using Info = std::unique_ptr<T, NET_API_STATUS(*)(LPVOID)>;

// -----------------------------------------------------------------------------

template<class T>
Info<T> workstation_info(const LMSTR server_name = {})
{
  using D = std::decay_t<T>;
  constexpr DWORD level = []
  {
    if constexpr (std::is_same_v<D, WKSTA_INFO_100>) {
      return 100;
    } else if constexpr (std::is_same_v<D, WKSTA_INFO_101>) {
      return 101;
    } else if constexpr (std::is_same_v<D, WKSTA_INFO_102>) {
      return 102;
    } else
      static_assert(false_value<D>);
  }();

  LPBYTE buf{};
  if (const auto e = NetWkstaGetInfo(server_name, level, &buf); e != NERR_Success)
    throw Sys_exception{e, "cannot get workstation network information"};
  return Info<T>{reinterpret_cast<D*>(buf), &NetApiBufferFree};
}

// -----------------------------------------------------------------------------

template<Throw_modifier Tm = throw_all>
Info<LOCALGROUP_INFO_1> local_group_info_1(const std::wstring& group_name,
  const std::wstring& server_name = {})
{
  const LPCWSTR server{!server_name.empty() ? server_name.c_str() : nullptr};
  LPBYTE buf{};
  const auto e = NetLocalGroupGetInfo(server, group_name.c_str(), 1, &buf);
  if (e != NERR_Success) {
    if constexpr (Tm == no_throw_if_not_found) {
      if (e == NERR_GroupNotFound)
        return {nullptr, nullptr};
    }
    throw Sys_exception{e, "cannot get local group info of level 1"};
  }
  return Info<LOCALGROUP_INFO_1>{
    reinterpret_cast<LOCALGROUP_INFO_1*>(buf), &NetApiBufferFree};
}

template<typename ... Types>
inline auto local_group_info_1_if_exists(Types&& ... args)
{
  return local_group_info_1<no_throw_if_not_found>(std::forward<Types>(args)...);
}

// -----------------------------------------------------------------------------

inline void local_group_add_members(const std::wstring& group_name,
  std::vector<LOCALGROUP_MEMBERS_INFO_0> members,
  const std::wstring& server_name = {})
{
  const LPCWSTR server{!server_name.empty() ? server_name.c_str() : nullptr};

  const auto err = NetLocalGroupAddMembers(server, group_name.c_str(),
    0, reinterpret_cast<LPBYTE>(members.data()), static_cast<DWORD>(members.size()));
  if (err != NERR_Success)
    throw Sys_exception{err, "cannot add group members"};
}

inline void local_group_add_members(const std::wstring& group_name,
  const std::vector<PSID>& members,
  const std::wstring& server_name = {})
{
  std::vector<LOCALGROUP_MEMBERS_INFO_0> mmbrs;
  mmbrs.reserve(members.size());
  for (const auto psid : members)
    mmbrs.emplace_back(psid);
  local_group_add_members(group_name, detail::to_vector_lgmi0(members),
    server_name);
}

// -----------------------------------------------------------------------------

inline void local_group_del_members(const std::wstring& group_name,
  std::vector<LOCALGROUP_MEMBERS_INFO_0> members,
  const std::wstring& server_name = {})
{
  const LPCWSTR server{!server_name.empty() ? server_name.c_str() : nullptr};

  const auto err = NetLocalGroupDelMembers(server, group_name.c_str(),
    0, reinterpret_cast<LPBYTE>(members.data()), static_cast<DWORD>(members.size()));
  if (err != NERR_Success)
    throw Sys_exception{err, "cannot remove group members"};
}

inline void local_group_del_members(const std::wstring& group_name,
  const std::vector<PSID>& members,
  const std::wstring& server_name = {})
{
  local_group_del_members(group_name, detail::to_vector_lgmi0(members),
    server_name);
}

} // namespace dmitigr::winbase::netman
