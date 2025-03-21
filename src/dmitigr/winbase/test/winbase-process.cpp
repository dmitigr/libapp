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

#include "../process.hpp"

#include <iostream>
#include <string>

#define ASSERT DMITIGR_ASSERT

int main(const int argc, const char* const* argv)
{
  using std::cout;
  using std::clog;
  using std::wcout;
  using std::endl;
  namespace win = dmitigr::winbase;

  try {
    DWORD pid{};
    if (argc > 1)
      pid = std::stoi(argv[1]);
    const auto hdl = win::open_process(pid,
      PROCESS_QUERY_LIMITED_INFORMATION, false);
    if (hdl)
      std::wcout << win::query_full_process_image_name(hdl) << std::endl;
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
