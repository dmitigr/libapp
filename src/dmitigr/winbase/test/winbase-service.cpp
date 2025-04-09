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

#include "../../base/assert.hpp"
#include "../exceptions.hpp"
#include "../service.hpp"

#include <cstdint>
#include <iostream>
#include <memory>

#define ASSERT DMITIGR_ASSERT

int main()
{
  using std::cout;
  using std::clog;
  using std::wcout;
  using std::endl;
  using std::uintptr_t;
  namespace dmwin = dmitigr::winbase;
  namespace dmsvc = dmitigr::winbase::service;

  try {
    const auto man = dmsvc::open_manager(SC_MANAGER_CONNECT);
    const auto svc = dmsvc::open_service(man, L"Schedule", SERVICE_QUERY_CONFIG);

    const dmsvc::Service_config cfg1{svc};
    const auto* const qsc1 = cfg1.ptr();
    const dmsvc::Service_config cfg2{svc};
    const auto* const qsc2 = cfg2.ptr();
    ASSERT((uintptr_t)cfg1->lpBinaryPathName != (uintptr_t)cfg2->lpBinaryPathName);

    wcout << std::hex;
    for (const auto* const qsc : {qsc1, qsc2}) {
      wcout << "struct offset: "
            << (uintptr_t)qsc << std::endl;
      wcout << "lpBinaryPathName offset: "
            << (uintptr_t)std::addressof(qsc->lpBinaryPathName) << std::endl;
      wcout << "lpDisplayName offset: "
            << (uintptr_t)std::addressof(qsc->lpDisplayName) << std::endl;
      wcout << "*lpBinaryPathName offset: "
            << (uintptr_t)qsc->lpBinaryPathName << std::endl;
      wcout << "lpBinaryPathName: "
            << qsc->lpBinaryPathName << std::endl;
      wcout << std::endl;
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
