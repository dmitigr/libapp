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

#include "../../base/assert.hpp"
#include "../account.hpp"
#include "../netman.hpp"
#include "../security.hpp"
#include "../strconv.hpp"

#include <iostream>

#define ASSERT DMITIGR_ASSERT

int main()
{
  using std::cout;
  using std::clog;
  using std::wcout;
  using std::endl;
  namespace dmwin = dmitigr::winbase;
  using dmwin::utf16_to_utf8;

  try {
    const dmwin::Sid rdp_sid{SECURITY_NT_AUTHORITY,
      SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS};
    const auto grp = dmwin::Account::make_if_mapped(rdp_sid.ptr());
    if (!grp) {
      cout << "Remote desktop is not available on this computer" << endl;
      return 0;
    } else {
      const auto grpinfo = dmwin::netman::local_group_info_1_if_exists(grp.name());
      if (grpinfo)
        cout << "Found remote desktop group \""
             << utf16_to_utf8(grpinfo->lgrpi1_name) << "\"" << endl;
    }
    const dmwin::Account user{L"dmitigr"};

    try {
      dmwin::netman::local_group_add_members(grp.name(), {user.sid()});
    } catch (const dmwin::Sys_exception& e) {
      if (e.code().value() == ERROR_MEMBER_IN_ALIAS)
        cout<<utf16_to_utf8(user.name())
            <<" is already in group "
            <<"\""<<utf16_to_utf8(grp.name())<<"\""<<endl;
    }
    try {
      dmwin::netman::local_group_del_members(grp.name(), {user.sid()});
    } catch (const dmwin::Sys_exception& e) {
      if (e.code().value() == ERROR_MEMBER_NOT_IN_ALIAS)
        cout<<utf16_to_utf8(user.name())
            <<" not in group "
            <<"\""<<utf16_to_utf8(grp.name())<<"\""<<endl;
    }
  } catch (const dmwin::Sys_exception& e) {
    const auto code = e.code().value();
    clog << "error "<<code<<": " << e.what() << endl;
  } catch (const std::exception& e) {
    clog << "error: " << e.what() << endl;
    return 1;
  } catch (...) {
    clog << "unknown error" << endl;
    return 2;
  }
}
