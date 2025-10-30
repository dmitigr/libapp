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
#include "../../pgfe/composite.hpp"
#include "../../pgfe/connection.hpp"
#include "../../pgfe/exceptions.hpp"
#include "../../pgfe/statement.hpp"
#include "pgfe-unit.hpp"

#include <functional>

template<typename ... Types>
void state_task(dmitigr::pgfe::Connection& dbconn, Types&& ... execute_args)
{
  static const dmitigr::pgfe::Statement call_set_state{R"(
select foo(
  id := :{id},
  state := :{state},
  descr := :{descr},
  curuser := curuser())
)"};

  dbconn.connect(); // just in case
  dbconn.execute(call_set_state, std::forward<Types>(execute_args)...);
}

int main()
{
  try {
    namespace pgfe = dmitigr::pgfe;

    const auto conn = pgfe::test::make_connection();
    conn->connect();

    {
      pgfe::Statement st;
      DMITIGR_ASSERT(st.is_empty());

      // append
      st = (R"(
      /*
       * $id$unknown-query$id$
       */)");
      DMITIGR_ASSERT(!st.is_empty());
      DMITIGR_ASSERT(st.is_query_empty());

      st.extra().append("description", std::string{"This is an unknown query"});
      DMITIGR_ASSERT(st.extra().size() == 1);
      DMITIGR_ASSERT(st.extra().index("description") != st.extra().size());
      DMITIGR_ASSERT(st.extra().value("description").has_value());

      st.append("SELECT 1");
      DMITIGR_ASSERT(st.extra().size() == 2);
      DMITIGR_ASSERT(st.extra().index("id") != st.extra().size());
      DMITIGR_ASSERT(st.extra().value("id").has_value());
      DMITIGR_ASSERT(std::any_cast<std::string>(st.extra().value("id")) == "unknown-query");
    }

    {
      pgfe::Statement st{
        "-- Id: simple\r\n"
        "SELECT /* comment */ 1::integer /*, $1::integer*/"};

      DMITIGR_ASSERT(st.positional_parameter_count() == 0);
      DMITIGR_ASSERT(st.named_parameter_count() == 0);
      DMITIGR_ASSERT(st.parameter_count() == 0);
      DMITIGR_ASSERT(!st.has_positional_parameter());
      DMITIGR_ASSERT(!st.has_named_parameter());
      DMITIGR_ASSERT(!st.has_parameter());

      DMITIGR_ASSERT(!st.is_empty());
      DMITIGR_ASSERT(!st.has_missing_parameter());

      std::cout << st.to_string() << std::endl;
    }

    {
      pgfe::Statement st{R"(SELECT :{num}, :{num}, :'txt', :'txt' FROM :"tab", :"tab")"};
      DMITIGR_ASSERT(!st.is_empty());
      DMITIGR_ASSERT(st.positional_parameter_count() == 0);
      DMITIGR_ASSERT(st.named_parameter_count() == 3);
      DMITIGR_ASSERT(st.parameter_count() == 3);
      DMITIGR_ASSERT(!st.has_positional_parameter());
      DMITIGR_ASSERT(st.has_named_parameter());
      DMITIGR_ASSERT(st.has_parameter());
      DMITIGR_ASSERT(!st.has_missing_parameter());
      DMITIGR_ASSERT(!st.is_parameter_literal("num"));
      DMITIGR_ASSERT(!st.is_parameter_identifier("num"));
      DMITIGR_ASSERT(st.is_parameter_literal("txt"));
      DMITIGR_ASSERT(st.is_parameter_identifier("tab"));

      st.replace("num", "1");
      DMITIGR_ASSERT(st.named_parameter_count() == 2);
      DMITIGR_ASSERT(st.parameter_count() == 2);

      DMITIGR_ASSERT(st.bound_parameter_count() == 0);
      DMITIGR_ASSERT(!st.has_bound_parameter());
      st.bind("txt", "one");
      DMITIGR_ASSERT(*st.bound("txt") == "one");
      std::cout << st.bound_parameter_count() << std::endl;
      DMITIGR_ASSERT(st.bound_parameter_count() == 1);
      DMITIGR_ASSERT(st.has_bound_parameter());
      st.bind("tab", "number");
      DMITIGR_ASSERT(*st.bound("tab") == "number");
      DMITIGR_ASSERT(st.bound_parameter_count() == 2);
      DMITIGR_ASSERT(st.has_bound_parameter());

      std::cout << st.to_string() << std::endl;
      std::cout << st.to_query_string(*conn) << std::endl;
    }

    {
      pgfe::Statement st{R"(SELECT 1 WHERE :{condition} and task_id = :{task_id})"};
      const auto quoted = conn->to_quoted_literal("SSH");
      st.bind("condition", "upper(code) = upper(" + quoted + ")");
      std::cout << st.to_string() << std::endl;
      std::cout << st.to_query_string(*conn) << std::endl;
      std::cout << st.parameter_count() << std::endl;
    }

    {
      const pgfe::Statement call{R"(
call adb_nsi.rmm_workstation_task_set_state(
  p_workstation_task_id := :{id},
  p_state_code := :{state},
  p_description := :{descr},
  p_current_user := adb_system.get_system_user_login())
)"};
      std::cout << call.to_string() << std::endl;
      std::cout << call.to_query_string(*conn) << std::endl;
      std::cout << call.parameter_count() << std::endl;
      DMITIGR_ASSERT(call.parameter_name(0) == "id");
      DMITIGR_ASSERT(call.parameter_name(1) == "state");
      DMITIGR_ASSERT(call.parameter_name(2) == "descr");

      // using pgfe::a;
      // const a a_task_id{"id", 12345};
      // state_task(*conn, a_task_id, a{"state", "TASK_COMPLETED"}, a{"descr", ""});
    }

    {
      const dmitigr::pgfe::Statement stmt{R"(
update task_step_log
  set end_date = now(), log_json = :{output}/*, error_text = :{errtext}*/
  where id = :{id}
  returning id
)"};
      DMITIGR_ASSERT(stmt.positional_parameter_count() == 0);
      DMITIGR_ASSERT(stmt.named_parameter_count() == 2);
      DMITIGR_ASSERT(stmt.parameter_count() == 2);
      std::cout << stmt.to_query_string(*conn) << std::endl;
    }

    {
      pgfe::Statement s_orig{
        "-- Id: complex\n"
        "SELECT :{last_name}::text, /* comment */ :{age}, $2, f(:{age}),"
        " 'simple string', $$dollar quoted$$, $tag$dollar quoted$tag$"};
      auto s_copy = s_orig;

      for (const auto& ref : {std::ref(s_orig), std::ref(s_copy)}) {
        const auto& st = ref.get();
        DMITIGR_ASSERT(st.positional_parameter_count() == 2);
        DMITIGR_ASSERT(st.named_parameter_count() == 2);
        DMITIGR_ASSERT(st.parameter_count() == (st.positional_parameter_count() + st.named_parameter_count()));
        DMITIGR_ASSERT(st.parameter_name(2) == "last_name");
        DMITIGR_ASSERT(st.parameter_name(3) == "age");
        DMITIGR_ASSERT(st.parameter_index("last_name") == 2);
        DMITIGR_ASSERT(st.parameter_index("age") == 3);
        DMITIGR_ASSERT(st.has_positional_parameter());
        DMITIGR_ASSERT(st.has_named_parameter());
        DMITIGR_ASSERT(st.has_parameter());

        DMITIGR_ASSERT(!st.is_empty());
        DMITIGR_ASSERT(st.is_parameter_missing(0));
        DMITIGR_ASSERT(st.has_missing_parameter());
      }

      for (auto& ref : {std::ref(s_orig), std::ref(s_copy)}) {
        auto& st = ref.get();
        st.append(" WHERE $1");
        DMITIGR_ASSERT(!st.is_parameter_missing(0));
        DMITIGR_ASSERT(!st.has_missing_parameter());
      }

      for (auto& ref : {std::ref(s_orig), std::ref(s_copy)}) {
        auto& st = ref.get();
        st.replace("age", "g(:{first_name}, :{age}, :{p2}) + 1");
        DMITIGR_ASSERT(st.parameter_index("first_name") == 3);
        DMITIGR_ASSERT(st.parameter_index("age") == 4);
        DMITIGR_ASSERT(st.parameter_index("p2") == 5);
      }

      std::cout << "Final SQL string is: " << s_orig.to_string() << std::endl;
    }

    // -------------------------------------------------------------------------
    // Parser tests
    // -------------------------------------------------------------------------

    // Dollar quoting.
    {
      const pgfe::Statement s1{"select $$hello $$$"};
      const pgfe::Statement s2{"select $$hello $a$$"};
      const pgfe::Statement s3{"select $$hello $a$ $$"};
      const pgfe::Statement s4{"select $abc$hello $ab$$abc$"};
    }


    // -------------------------------------------------------------------------
    // Comparison
    // -------------------------------------------------------------------------

    {
      const pgfe::Statement s1{
        "-- Id: complex\n"
        "SELECT :{last_name}::text, /* comment */ :{age}, $2, f(:{age}),"
        " 'simple string', $$dollar quoted$$, $tag$dollar quoted$tag$"};
      const pgfe::Statement s2{
        "select  \n"
        ":{last_name}  ::TEXT\n"
        ", \n"
        "/* another comment */ :{age}, $2, F(:{age}),"
        " 'simple string', $$dollar quoted$$, $tag$dollar quoted$tag$"};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select 1"};
      const pgfe::Statement s2{"select 2"};
      DMITIGR_ASSERT(!s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(!s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select :{a} /*a*/"};
      const pgfe::Statement s2{"select :{b} /*b*/"};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(!s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(!s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select :{a} /*a*/"};
      const pgfe::Statement s2{"select :{a} /*a*/"};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select /*one*/ 1"};
      const pgfe::Statement s2{"SELECT 1"};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select /*one*/ 1"};
      const pgfe::Statement s2{"SELECT 1 /*one*/"};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select 1"};
      const pgfe::Statement s2{"select1"};
      DMITIGR_ASSERT(!s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(!s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select 1"};
      const pgfe::Statement s2{"select 1, :{two}"};
      DMITIGR_ASSERT(!s1.is_same(s2));
      DMITIGR_ASSERT(!s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(!s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"select /*comment*/ :{two}, 1"};
      const pgfe::Statement s2{"select /*comment*/ 1, :{two}"};
      DMITIGR_ASSERT(!s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(!s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"   select  1"};
      const pgfe::Statement s2{"select    1 "};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

    {
      const pgfe::Statement s1{"SELECT ARRAY [  [1:2], [3:4]]"};
      const pgfe::Statement s2{"select   array[[1:2],[3:4]] "};
      DMITIGR_ASSERT(s1.is_same(s2));
      DMITIGR_ASSERT(s1.is_named_parameters_equal(s2));
      DMITIGR_ASSERT(s1.is_equal(s2));
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
