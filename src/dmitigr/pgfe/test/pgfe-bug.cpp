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

inline void state_task(dmitigr::pgfe::Connection& dbconn,
  const std::int64_t id,
  const std::string& state,
  const std::string& descr = {},
  const std::string& login = {})
{
  static const dmitigr::pgfe::Statement call_set_state{R"(
call set_state(
  p_task_step_log_id := :id::bigint,
  p_state_code := :state::varchar,
  p_description := :descr::varchar,
  p_current_user := :login::varchar)
)"};

  dbconn.connect();

  const dmitigr::pgfe::a a_id{"id", id}, a_state{"state", state}, a_descr{"descr", descr};
  if (login.empty()) {
    auto query = call_set_state;
    query.bind("login", "current_user");
    dbconn.execute(query, a_id, a_state, a_descr);
    std::cout << query.to_query_string(dbconn) << std::endl;
  } else
    dbconn.execute(call_set_state, a_id, a_state, a_descr,
      dmitigr::pgfe::a{"login", login});

  const dmitigr::pgfe::Statement update_task{R"(
update task_step_log
  set end_date = now(), log_json = :output/*, error_text = :errtext*/
  where id = :id
  returning id
)"};
  std::cout << update_task.to_query_string(dbconn) << std::endl;
}

int main()
{
  namespace test = dmitigr::pgfe::test;
  auto conn = test::make_connection();
  state_task(*conn, 1, "ERROR");
}
