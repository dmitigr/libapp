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

#include "../../base/assert.hpp"
#include "../../http/basics.hpp"

int main()
{
  try {
    namespace http = dmitigr::http;
    using http::Same_site;
    using http::to_same_site;
    using http::to_string_view;
    {
      DMITIGR_ASSERT(to_same_site("Strict") == Same_site::strict);
      DMITIGR_ASSERT(to_same_site("Lax") == Same_site::lax);
      DMITIGR_ASSERT(to_string_view(Same_site::strict) == "Strict");
      DMITIGR_ASSERT(to_string_view(Same_site::lax) == "Lax");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
