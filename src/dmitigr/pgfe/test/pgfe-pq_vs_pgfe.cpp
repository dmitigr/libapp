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

#include <libpq-fe.h>

#include <new>

const char query[] = "select generate_series(1,1000000)";

struct Result final {
  int length{};
  int format{};
  char* value{};
  int is_null{};
  std::unique_ptr<PGresult> res_;
};

inline auto result(PGresult* const res) noexcept
{
  Result r;
  r.length = PQgetlength(res, 0, 0);
  r.format = PQfformat(res, 0);
  r.value = PQgetvalue(res, 0, 0);
  r.is_null = PQgetisnull(res, 0, 0);
  r.res_.reset(res);
  return r;
}

inline void test_pq()
{
  auto* const conn = PQconnectdb("hostaddr=127.0.0.1 user=pgfe_test"
    " password=pgfe_test dbname=pgfe_test connect_timeout=7");

  if (!conn)
    throw std::bad_alloc{};

  if (PQstatus(conn) != CONNECTION_OK) {
    PQfinish(conn);
    throw std::runtime_error{"cannot connect to server"};
  }

  if (!PQsendQuery(conn, query)) {
    PQfinish(conn);
    throw std::runtime_error{"cannot send query"};
  }

  if (!PQsetSingleRowMode(conn)) {
    PQfinish(conn);
    throw std::runtime_error{"cannot switch to single row mode"};
  }

  while (auto* const res = PQgetResult(conn)) {
    switch (PQresultStatus(res)) {
    case PGRES_TUPLES_OK:
      PQclear(res);
      break;
    case PGRES_SINGLE_TUPLE: {
      auto r = result(res);
      break;
    } default:
      PQclear(res);
      PQfinish(conn);
      throw std::runtime_error{PQresultErrorMessage(res)};
    }
  }

  PQfinish(conn);
}

inline void test_pgfe()
{
  namespace pgfe = dmitigr::pgfe;
  const auto conn = pgfe::Connection::make(pgfe::Connection_options{}
    .set(pgfe::Communication_mode::net)
    .set_address("127.0.0.1")
    .set_username("pgfe_test")
    .set_password("pgfe_test")
    .set_database("pgfe_test")
    .set_connect_timeout(std::chrono::seconds{7}));
  conn->connect();
  conn->execute([](auto&& r) { auto d = r.data(); }, query);
}

int main()
{
  using dmitigr::call;

  std::chrono::milliseconds elapsed;

  std::cout << "Pq: ";
  call(elapsed, test_pq);
  std::cout << elapsed.count() << std::endl;

  std::cout << "Pgfe: ";
  call(elapsed, test_pgfe);
  std::cout << elapsed.count() << std::endl;
}
