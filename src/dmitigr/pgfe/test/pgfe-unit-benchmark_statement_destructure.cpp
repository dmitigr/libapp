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

#include "../../pgfe/statement.hpp"

#include <iostream>

int main(const int argc, char* const argv[])
try {
  namespace pgfe = dmitigr::pgfe;
  using pgfe::Statement;

  const unsigned long iteration_count{(argc >= 2) ? std::stoul(argv[1]) : 1};
  const Statement pattern{"with :{with_name} as (:{subquery}) :{query}"};
  const Statement stmt{"  with  foo  as (  select 1, 2 ) select 3 , 4 "};
  for (unsigned long i{}; i < iteration_count; ++i)
    stmt.destructure([](const auto& name, const auto& match)
    {
      return !name.empty() && !match.is_empty();
    }, pattern);
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
