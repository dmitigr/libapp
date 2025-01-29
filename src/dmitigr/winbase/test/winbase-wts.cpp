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
#include "../wts.hpp"

#include <iostream>

#define ASSERT DMITIGR_ASSERT

int main()
{
  try {
    using std::cout;
    using std::wcout;
    using std::endl;
    namespace wts = dmitigr::winbase::wts;

    wts::Session_enumeration se{WTS_CURRENT_SERVER_HANDLE};
    for (DWORD i{}; i < se.count(); ++i) {
      const auto& info = se.info()[i];
      wcout << "Session"<<i
            <<": ExecEnvId="<<info.ExecEnvId
            <<", SessionId="<<info.SessionId
            <<", SessionName="<<(info.pSessionName ? info.pSessionName : L"")
            <<", State="<<info.State
            <<", HostName="<<(info.pHostName ? info.pHostName : L"")
            <<", UserName="<<(info.pUserName ? info.pUserName : L"")
            << endl;
    }
  } catch (const std::exception& e) {
    std::clog << "error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::clog << "unknown error" << std::endl;
    return 2;
  }
}
