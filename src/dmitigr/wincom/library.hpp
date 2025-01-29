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
#pragma comment(lib, "ole32")

#include "../base/noncopymove.hpp"
#include "../winbase/windows.hpp"
#include "exceptions.hpp"

#include <Objbase.h>

namespace dmitigr::wincom {

class Library final : private Noncopymove {
public:
  ~Library()
  {
    CoUninitialize();
  }

  Library()
    : Library{COINIT_MULTITHREADED}
  {}

  explicit Library(const DWORD concurrency_model)
  {
    const auto err = CoInitializeEx(nullptr, concurrency_model);
    if (err != S_OK && err != S_FALSE)
      throw Win_error{"cannot initialize COM library", err};
  }
};

/**
 * @param auth A value of `-1` tells COM to choose authentication services
 * to register.
 * @param auth_services Must be `nullptr` if `auth` is set to `-1`.
 */
inline void initialize_security(const PSECURITY_DESCRIPTOR sec_desc = {},
  const DWORD auth = -1,
  SOLE_AUTHENTICATION_SERVICE* const auth_services = {},
  const DWORD auth_level = RPC_C_AUTHN_LEVEL_DEFAULT,
  const DWORD imperson_level = RPC_C_IMP_LEVEL_IMPERSONATE,
  SOLE_AUTHENTICATION_LIST* const auth_list = {},
  const DWORD capabilities = EOAC_NONE)
{
  const auto err = CoInitializeSecurity(sec_desc, auth, auth_services,
    nullptr/*reserved*/, auth_level, imperson_level, auth_list, capabilities,
    nullptr/*reserved*/);
  if (err == RPC_E_TOO_LATE)
    return;
  else
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot register security and set the"
      " default security values for the process");
}

/**
 * @param proxy The proxy to set.
 * @param authn Authentication service.
 * @param authz Authorization service.
 * @param server_princ_name Server principal name.
 * @param authn_level Authentication level.
 * @param imperson_level Impersonation level.
 * @param auth_info Client identity.
 * @param capabilities Proxy capabilities.
 */
inline void set_proxy_blanket(IUnknown* const proxy,
  const DWORD authn = RPC_C_AUTHN_DEFAULT,
  const DWORD authz = RPC_C_AUTHZ_DEFAULT,
  OLECHAR* const server_princ_name = nullptr,
  const DWORD authn_level = RPC_C_AUTHN_LEVEL_DEFAULT,
  const DWORD imperson_level = RPC_C_IMP_LEVEL_DEFAULT,
  const RPC_AUTH_IDENTITY_HANDLE auth_info = nullptr,
  const DWORD capabilities = EOAC_DEFAULT)
{
  const auto err = CoSetProxyBlanket(proxy, authn, authz, server_princ_name,
    authn_level, imperson_level, auth_info, capabilities);
  DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot sets the authentication info that"
    " will be used to make calls on the specified proxy");
}

} // namespace dmitigr::wincom
