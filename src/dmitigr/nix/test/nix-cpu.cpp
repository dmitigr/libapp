// -*- C++ -*-
//
// Copyright 2026 Dmitry Igrishin
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

#include "../cpu.hpp"
#include "../../base/assert.hpp"
#include "../../base/str.hpp"

#include <iostream>

int main()
{
  try {
    namespace nix = dmitigr::nix;
    namespace str = dmitigr::str;
    using std::cout;
    using std::endl;
    cout << "SMT is " << to_string_view(nix::smt_status()) << '.' << endl;

    nix::for_each_cpu([](auto&& cpu)
    {
      DMITIGR_ASSERT(cpu.is_valid());
      cout << std::boolalpha
           << endl
           << "CPU " << cpu.index() << ":" << endl
           << "  Physical: " << cpu.is_physical() << endl
           << "  Performant: " << cpu.is_performant() << endl
           << "  Capacity: " << cpu.capacity() << endl
           << "  Core list: " << str::to_string([](const auto& range)
           {
             return range.to_string();
           }, cpu.core_list(), ",")
           << endl << endl;
      return true;
    });
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
