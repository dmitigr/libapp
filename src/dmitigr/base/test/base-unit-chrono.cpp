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
#include "../../base/chrono.hpp"

#include <iostream>
#include <limits>

int main()
{
  try {
    namespace chrono = dmitigr::chrono;

    std::cout.precision(std::numeric_limits<long int>::max_digits10);
    std::cout << chrono::now() << std::endl;
    std::cout << chrono::now_us() << std::endl;
    std::cout << chrono::now_us() << std::endl;
    std::cout << chrono::now_iso8601() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
