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
try {
  namespace pgfe = dmitigr::pgfe;
  using pgfe::to;
  using pgfe::Data_view;

  const auto conn = pgfe::test::make_connection();
  conn->connect();
  DMITIGR_ASSERT(conn->is_connected());

  {
    auto ps1 = conn->prepare_as_is("SELECT $1::integer", "ps1");
    DMITIGR_ASSERT(ps1 && ps1.name() == "ps1");
    DMITIGR_ASSERT(!ps1.is_preparsed());
    DMITIGR_ASSERT(!ps1.is_described());
    DMITIGR_ASSERT(!ps1.has_parameter());
    DMITIGR_ASSERT(!ps1.has_named_parameter());
    DMITIGR_ASSERT(!ps1.has_positional_parameter());
    DMITIGR_ASSERT(ps1.parameter_count() == 0);
    ps1.bind(64, 1983);
    DMITIGR_ASSERT(ps1.parameter_count() == 65);
    DMITIGR_ASSERT(ps1.positional_parameter_count() == 65);

    try {
      ps1.execute();
    } catch (const pgfe::Sqlstate_exception& e) {
      DMITIGR_ASSERT(e.error()->condition() == pgfe::Sqlstate::c08_protocol_violation);
    }

    ps1.describe();
    DMITIGR_ASSERT(ps1.is_described());
    DMITIGR_ASSERT(ps1.parameter_count() == 1);
    DMITIGR_ASSERT(ps1.positional_parameter_count() == 1);
    DMITIGR_ASSERT(!ps1.bound(0));
    ps1.bind(0, 1983);
    ps1.execute([](auto&& row)
    {
      DMITIGR_ASSERT(row[0]);
      DMITIGR_ASSERT(pgfe::to<int>(row[0]) == 1983);
    });
  }

  static const pgfe::Statement ss{
    "SELECT 1::integer AS const,"
      " generate_series(:infinum::integer, :supremum::integer) AS var,"
      " 2::integer AS const"};
  auto ps2 = conn->prepare(ss, "ps2");
  DMITIGR_ASSERT(ps2 && ps2.name() == "ps2");
  DMITIGR_ASSERT(ps2.is_preparsed());
  DMITIGR_ASSERT(!ps2.is_described());
  DMITIGR_ASSERT(ps2.positional_parameter_count() == 0);
  DMITIGR_ASSERT(ps2.named_parameter_count() == 2);
  DMITIGR_ASSERT(ps2.parameter_count() == 2);
  DMITIGR_ASSERT(ps2.parameter_name(0) == "infinum");
  DMITIGR_ASSERT(ps2.parameter_name(1) == "supremum");
  DMITIGR_ASSERT(ps2.parameter_index("infinum") == 0);
  DMITIGR_ASSERT(ps2.parameter_index("supremum") == 1);
  DMITIGR_ASSERT(ps2.has_parameter("infinum"));
  DMITIGR_ASSERT(ps2.has_parameter("supremum"));
  DMITIGR_ASSERT(!ps2.has_positional_parameter());
  DMITIGR_ASSERT(ps2.has_named_parameter());
  DMITIGR_ASSERT(ps2.has_parameter());
  //
  DMITIGR_ASSERT(ps2.name() == "ps2");
  DMITIGR_ASSERT(!ps2.bound(0));
  DMITIGR_ASSERT(!ps2.bound(1));
  DMITIGR_ASSERT(!ps2.bound("infinum"));
  DMITIGR_ASSERT(!ps2.bound("supremum"));
  ps2.bind("infinum", 1);
  ps2.bind("supremum", 3);
  DMITIGR_ASSERT(ps2.bound(0) && to<int>(ps2.bound(0)) == 1);
  DMITIGR_ASSERT(ps2.bound(1) && to<int>(ps2.bound(1)) == 3);
  const auto data0 = pgfe::Data::make("1");
  const auto data1 = pgfe::Data::make("3");
  ps2.bind("infinum", *data0);
  ps2.bind("supremum", *data1);
  DMITIGR_ASSERT(ps2.bound(0) == *data0);
  DMITIGR_ASSERT(ps2.bound(1) == *data1);
  ps2.bind("infinum", nullptr);
  ps2.bind("supremum", nullptr);
  DMITIGR_ASSERT(!ps2.bound(0));
  DMITIGR_ASSERT(!ps2.bound(1));
  ps2.bind_many(1, 3);
  DMITIGR_ASSERT(ps2.bound(0) && to<int>(ps2.bound(0)) == 1);
  DMITIGR_ASSERT(ps2.bound(1) && to<int>(ps2.bound(1)) == 3);
  //
  DMITIGR_ASSERT(ps2.result_format() == conn->result_format());
  DMITIGR_ASSERT(&ps2.connection() == conn.get());
  DMITIGR_ASSERT(!ps2.is_described());
  DMITIGR_ASSERT(!ps2.parameter_type_oid(0));
  DMITIGR_ASSERT(!ps2.row_info());
  //
  ps2.describe();
  DMITIGR_ASSERT(ps2.is_described());
  constexpr std::uint_fast32_t integer_oid{23};
  DMITIGR_ASSERT(ps2.parameter_type_oid(0) == integer_oid);
  DMITIGR_ASSERT(ps2.parameter_type_oid(1) == integer_oid);
  const auto& ri = ps2.row_info();
  DMITIGR_ASSERT(ri);
  DMITIGR_ASSERT(ri.field_count() == 3);
  DMITIGR_ASSERT(!ri.is_empty());
  DMITIGR_ASSERT(ri.field_name(0) == "const");
  DMITIGR_ASSERT(ri.field_name(1) == "var");
  DMITIGR_ASSERT(ri.field_name(2) == "const");
  DMITIGR_ASSERT(ri.field_index("const")      == 0);
  DMITIGR_ASSERT(ri.field_index("var")        == 1);
  DMITIGR_ASSERT(ri.field_index("const", 1)   == 2);
  DMITIGR_ASSERT(ri.field_index("const") < ri.field_count());
  DMITIGR_ASSERT(ri.field_index("var") < ri.field_count());
  for (std::size_t i = 0; i < 3; ++i) {
    const auto fname = ri.field_name(i);
    DMITIGR_ASSERT(ri.table_oid(i)        == 0);
    DMITIGR_ASSERT(ri.table_oid(fname, i) == 0);
    DMITIGR_ASSERT(ri.table_column_number(i)        == 0);
    DMITIGR_ASSERT(ri.table_column_number(fname, i) == 0);
    DMITIGR_ASSERT(ri.type_oid(i)        == integer_oid);
    DMITIGR_ASSERT(ri.type_oid(fname, i) == integer_oid);
    DMITIGR_ASSERT(ri.type_size(i)        >= 0);
    DMITIGR_ASSERT(ri.type_size(fname, i) >= 0);
    DMITIGR_ASSERT(ri.type_modifier(i)        == -1);
    DMITIGR_ASSERT(ri.type_modifier(fname, i) == -1);
    DMITIGR_ASSERT(ri.data_format(i)        == pgfe::Data_format::text);
    DMITIGR_ASSERT(ri.data_format(fname, i) == pgfe::Data_format::text);
  }
  //
  ps2.execute([i = 1](auto&& row) mutable
  {
    DMITIGR_ASSERT(pgfe::to<int>(row[0]) == 1);
    DMITIGR_ASSERT(pgfe::to<int>(row[1]) == i);
    DMITIGR_ASSERT(pgfe::to<int>(row[2]) == 2);
    ++i;
  });

  // Test invalidation of prepared statements after disconnection.
  auto ps3 = conn->prepare("select 3", "ps3");
  auto ps3_2 = conn->describe("ps3");
  DMITIGR_ASSERT(ps3);
  DMITIGR_ASSERT(ps3_2);
  conn->disconnect();
  DMITIGR_ASSERT(!ps3);
  DMITIGR_ASSERT(!ps3_2);
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
