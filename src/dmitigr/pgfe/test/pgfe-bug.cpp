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

#include "pgfe-unit.hpp"

int main()
{
  namespace test = dmitigr::pgfe::test;
  auto conn = test::make_connection();
  conn->connect();
  const dmitigr::pgfe::Statement stmt{R"(
update task_step_log
  set end_date = now(), log_json = :output/*, error_text = :errtext*/
  where id = :id
  returning id
)"};
  DMITIGR_ASSERT(stmt.positional_parameter_count() == 0);
  DMITIGR_ASSERT(stmt.named_parameter_count() == 2);
  DMITIGR_ASSERT(stmt.parameter_count() == 2);
  std::cout << stmt.to_query_string(*conn) << std::endl;
}
