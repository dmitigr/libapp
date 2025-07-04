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

#include "../../str/stream.hpp"
#include "pgfe-unit.hpp"

int main(int, char* argv[])
try {
  namespace pgfe = dmitigr::pgfe;
  namespace str = dmitigr::str;
  using pgfe::to;

  // -------------------------------------------------------------------------
  // General test
  // -------------------------------------------------------------------------

  pgfe::Statement_vector bunch;
  DMITIGR_ASSERT(bunch.is_empty());
  DMITIGR_ASSERT(bunch.size() == 0);
  bunch.append("SELECT 1");
  DMITIGR_ASSERT(!bunch.is_empty());
  DMITIGR_ASSERT(bunch.size() == 1);
  DMITIGR_ASSERT(bunch.to_string() == "SELECT 1");
  const auto& vec = bunch.vector();
  DMITIGR_ASSERT(vec.size() == bunch.size());
  DMITIGR_ASSERT([&]
  {
    for (std::size_t i{}; i < vec.size(); ++i) {
      if (vec[i].to_string() != bunch[i].to_string())
        return false;
    }
    return true;
  }());

  // -------------------------------------------------------------------------
  // External SQL test
  // -------------------------------------------------------------------------

  const std::filesystem::path this_exe_file_name{argv[0]};
  const auto this_exe_dir_name = this_exe_file_name.parent_path();
  const auto input = str::read_to_string(this_exe_dir_name /
    "pgfe-unit-statement_vector.sql");
  bunch = pgfe::Statement_vector{input};
  DMITIGR_ASSERT(bunch.size() == 2);
  DMITIGR_ASSERT(bunch[0].extra().field_count() == 1);
  DMITIGR_ASSERT(bunch[1].extra().field_count() == 2);
  //
  DMITIGR_ASSERT(bunch.statement_index("id", "plus_one") == 0);
  DMITIGR_ASSERT(bunch.statement_index("id", "digit") == 1);
  DMITIGR_ASSERT(bunch[0].extra().field_index("id") == 0);
  DMITIGR_ASSERT(bunch[1].extra().field_index("id") == 0);
  DMITIGR_ASSERT(bunch[1].extra().field_index("cond") == 1);

  auto& digit = bunch[1];
  const auto& plus_one = bunch[0];
  const auto conn = pgfe::test::make_connection();
  conn->connect();

  // plus_one
  {
    conn->execute([](auto&& row)
    {
      DMITIGR_ASSERT(to<int>(row[0]) == 2 + 1);
    }, plus_one, 2);
  }

  // digit
  {
    DMITIGR_ASSERT(digit.has_parameter("cond"));
    DMITIGR_ASSERT(to<std::string>(digit.extra().data("cond")) == "n > 0\n  AND n < 2");
    digit.replace("cond", to<std::string_view>(digit.extra().data("cond")));
    conn->execute([](auto&& row)
    {
      DMITIGR_ASSERT(to<int>(row[0]) == 1);
    }, digit);
  }

  // -------------------------------------------------------------------------
  // Modifying the SQL vector
  // -------------------------------------------------------------------------

  bunch.insert(1, "SELECT 2");
  DMITIGR_ASSERT(bunch.size() == 3);
  auto i = bunch.statement_index("id", "plus_one");
  DMITIGR_ASSERT(i != bunch.size());
  bunch.remove(i);
  DMITIGR_ASSERT(bunch.size() == 2); // {"SELECT 2", digit} are still here
  DMITIGR_ASSERT(bunch.statement_index("id", "plus_one") == bunch.size());
  DMITIGR_ASSERT(bunch[0].to_string() == "SELECT 2"); // SELECT 2
  DMITIGR_ASSERT(bunch.statement_index("id", "digit") == 1);
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
