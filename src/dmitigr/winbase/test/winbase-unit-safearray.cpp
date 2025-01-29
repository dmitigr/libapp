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
#include "../combase.hpp"

#include <iostream>

#define ASSERT DMITIGR_ASSERT

int main()
{
  try {
    using std::cout;
    using std::wcout;
    using std::endl;
    namespace com = dmitigr::winbase::com;

    com::Safe_array arr{VT_VARIANT,
                        {{.cElements = 7, .lLbound = 0},
                         {.cElements = 2, .lLbound = 0},
                         {.cElements = 5, .lLbound = 0}}};
    auto sli = arr.slice().slice(4).slice(1);
    const auto es = arr.element_size();

    const auto sli_size = sli.size();
    const auto var = sli.array<VARIANT>();
    VariantInit(&var[3]);
    var[3].vt = VT_I4;
    var[3].intVal = 41;
    for (std::size_t i{}; i < sli.size(); ++i) {
      const auto v = var[i];
      if (v.vt == VT_I4)
        ASSERT(v.intVal == 41);
    }

    com::Variant v{var[3]};
    auto s = com::to<std::int64_t>(v);
    ASSERT(s == 41);
  } catch (const std::exception& e) {
    std::clog << "error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::clog << "unknown error" << std::endl;
    return 2;
  }
}
