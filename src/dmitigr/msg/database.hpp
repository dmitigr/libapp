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

#ifndef DMITIGR_MSG_DATABASE_HPP
#define DMITIGR_MSG_DATABASE_HPP

#include "../sqlixx/sqlixx.hpp"

#include <string_view>

namespace dmitigr::msg::db {

enum class Msg_status {
  unprocessed,
  processing,
  processed_ok,
  processed_error
};

/// Query for creating the `msg` table.
constexpr std::string_view create_msg_q{R"(
    create table if not exists msg(
      id                   integer not null primary key,
      ts                   integer not null, -- timestamp
      pid                  integer not null, -- PID
      status               integer not null,
      error                text,
      sender               text not null,
      recipients           text not null, -- JSON array
      subject              text not null,
      content              text not null)
  )"};

/// Quero for creating the indices on the `msg` table.
constexpr std::string_view create_msg_indices_q{R"(
create index if not exists msg_status_idx on msg(status)
)"};

constexpr std::string_view insert_msg_q{R"(
insert into msg(ts, pid, status, sender, recipients, subject, content)
  values(?, ?, ?, ?, ?, ?, ?)
)"};

inline const auto select_msg_q = std::string("select * from msg where status = ")
  .append(std::to_string(static_cast<int>(Msg_status::unprocessed)))
  .append(" or (status = ")
  .append(std::to_string(static_cast<int>(Msg_status::processing)))
  .append(" and pid <> ? and ts < ?) order by ts limit 1");

constexpr std::string_view update_msg_status_q{R"(
update msg set status = ? where id = ?
)"};

constexpr std::string_view update_msg_status_error_q{R"(
update msg set (status, error) = (?, ?) where id = ?
)"};

constexpr std::string_view update_msg_pid_q{R"(
update msg set pid = ? where id = ?
)"};

/// Sets pragmas suited for most cases.
inline void set_on_disk_pragmas(sqlixx::Connection& conn)
{
  conn.execute("pragma locking_mode = normal");
  conn.execute("pragma journal_mode = wal");
  // conn.execute("pragma wal_autocheckpoint = 100");
  // conn.execute("pragma cache_size = -64000");
  conn.execute("pragma synchronous = normal"); // off - maximum performance,
                                               // normal - maximum safety
}

} // namespace dmitigr::msg::db

namespace dmitigr::sqlixx {

template<>
struct Conversions<msg::db::Msg_status> final {
  static void bind(sqlite3_stmt* const handle, const int index,
    const msg::db::Msg_status value)
  {
    Conversions<int>::bind(handle, index, static_cast<int>(value));
  }

  static msg::db::Msg_status result(sqlite3_stmt* const handle, const int index)
  {
    return static_cast<msg::db::Msg_status>(Conversions<int>::result(handle, index));
  }
};

} // namespace dmitigr::sqlixx

#endif  // DMITIGR_MSG_DATABASE_HPP
