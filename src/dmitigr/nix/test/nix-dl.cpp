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
#include "../../nix/dl.hpp"

#include <iostream>

int main()
{
  try {
    namespace dl = dmitigr::nix::dl;

    dl::Object obj{"./libdmitigr_nix_dl.so", RTLD_NOW};
    const auto add = obj.symbol("add");
    const auto sub = obj.symbol("sub");
    const auto mul = obj.symbol("mul");
    DMITIGR_ASSERT(add);
    DMITIGR_ASSERT(sub);
    DMITIGR_ASSERT(!mul);
    DMITIGR_ASSERT(add.invoke<int>(10, 20) == 10 + 20);
    DMITIGR_ASSERT(sub.invoke<int>(10, 20) == 10 - 20);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
