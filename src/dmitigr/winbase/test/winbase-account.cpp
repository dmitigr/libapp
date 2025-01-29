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
  namespace win = dmitigr::winbase;

  try {
    const win::Sid sid{SECURITY_NT_AUTHORITY,
      SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DEFAULT_ACCOUNT};
    cout << win::utf16_to_utf8(win::Account{sid.ptr()}.name()) << endl;
  } catch (const win::Sys_exception& e) {
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
