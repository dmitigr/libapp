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

#ifndef DMITIGR_MSG_SQLITE_CONVEYOR_HPP
#define DMITIGR_MSG_SQLITE_CONVEYOR_HPP

#include "../os/pid.hpp"
#include "../rajson/conversions.hpp"
#include "conveyor.hpp"
#include "database.hpp"
#include "processor.hpp"
#include "message.hpp"

#include <algorithm>
#include <filesystem>
#include <mutex>

namespace dmitigr::msg {

class Sqlite_conveyor final : public Conveyor {
public:
  Sqlite_conveyor() = default;

  explicit Sqlite_conveyor(std::filesystem::path dbpath)
    : dbpath_{std::move(dbpath)}
    , pid_{os::pid()}
  {}

  void start() override
  {
    start_time_ = {};
    try {
      constexpr auto dbflags =
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;

      // Open the database.
      conn_ = {dbpath_, dbflags};

      // Create the database.
      conn_.execute(db::create_msg_q);
      conn_.execute(db::create_msg_indices_q);

      // Prepare the database.
      db::set_on_disk_pragmas(conn_);
      prepare_statements__();
    } catch (...) {
      conn_ = {};
      throw;
    }
    start_time_ = std::chrono::system_clock::now();
  }

  void stop() override
  {
    const std::lock_guard lg{mutex_};
    if (!is_started__())
      return;
    close_statements__();
    conn_.close();
    start_time_ = {};
  }

  bool is_started() const noexcept override
  {
    const std::lock_guard lg{mutex_};
    return is_started__();
  }

  void spool(const Message& message) override
  {
    if (message.sender.empty())
      throw Exception{Errc::message_sender_invalid,
        "cannot spool message: sender is invalid"};
    else if (message.recipients.empty() ||
      any_of(message.recipients.cbegin(), message.recipients.cend(),
        [](const auto& r){return r.empty();}))
      throw Exception{Errc::message_recipients_invalid,
        "cannot spool message: recipients are invalid"};
    else if (message.subject.empty())
      throw Exception{Errc::message_subject_invalid,
        "cannot spool message: subject is invalid"};
    else if (message.content.empty())
      throw Exception{Errc::message_content_invalid,
        "cannot spool message: content is invalid"};

    namespace chrono = std::chrono;
    const std::lock_guard lg{mutex_};
    const auto ts = chrono::system_clock::now().time_since_epoch();
    insert_msg_s_.execute(static_cast<sqlite_int64>(ts.count()),
      pid_,
      db::Msg_status::unprocessed,
      message.sender,
      rajson::to_text(
        rajson::to_value(message.recipients,
          rapidjson::Document{}.GetAllocator())),
      message.subject,
      message.content);
  }

  bool process(Processor& processor) override
  {
    sqlite_int64 id{-1};
    Message msg;
    {
      const std::lock_guard lg{mutex_};
      const auto r = select_msg_s_.execute(
        [this, &msg, &id](const sqlixx::Statement& stmt)mutable
        {
          DMITIGR_ASSERT(id == -1); // because of limit 1 in SQL query
          id = stmt.result<sqlite_int64>("id");
          const auto status = stmt.result<db::Msg_status>("status");
          switch (status) {
          case db::Msg_status::unprocessed:
            update_msg_status_s_.execute(db::Msg_status::processing, id);
            break;
          case db::Msg_status::processing:
            update_msg_pid_s_.execute(pid_, id);
            break;
          default:
            DMITIGR_ASSERT(false);
          }
          msg.sender = stmt.result<std::string>("sender");
          msg.recipients = rajson::to<std::vector<std::string>>(
            rajson::document_from_text(stmt.result<std::string_view>("recipients")));
          msg.subject = stmt.result<std::string>("subject");
          msg.content = stmt.result<std::string>("content");
        }, pid_, static_cast<sqlite_int64>(start_time_.time_since_epoch().count()));
      DMITIGR_ASSERT(r == SQLITE_DONE);
    }

    if (id < 0)
      return false; // no message to process

    try {
      const int error{processor.process(msg)};
      const std::lock_guard lg{mutex_};
      if (error)
        update_msg_status_error_s_.execute(db::Msg_status::processed_error,
          std::to_string(error), id);
      else
        update_msg_status_s_.execute(db::Msg_status::processed_ok, id);
    } catch (const std::exception& e) {
      const std::lock_guard lg{mutex_};
      update_msg_status_error_s_.execute(db::Msg_status::processed_error,
        e.what(), id);
      throw;
    } catch (...) {
      const std::lock_guard lg{mutex_};
      update_msg_status_error_s_.execute(db::Msg_status::processed_error,
        "unknown error", id);
      throw;
    }

    return true; // another one message was processed
  }

private:
  std::filesystem::path dbpath_;
  std::chrono::time_point<std::chrono::system_clock> start_time_{};
  os::Pid pid_{};
  mutable std::mutex mutex_;
  sqlixx::Connection conn_;
  sqlixx::Statement insert_msg_s_;
  sqlixx::Statement select_msg_s_;
  sqlixx::Statement update_msg_status_s_;
  sqlixx::Statement update_msg_status_error_s_;
  sqlixx::Statement update_msg_pid_s_;

  bool is_started__() const noexcept
  {
    return start_time_ > decltype(start_time_){};
  }

  void prepare_statements__()
  {
    DMITIGR_ASSERT(conn_);
    insert_msg_s_ = conn_.prepare(db::insert_msg_q);
    select_msg_s_ = conn_.prepare(db::select_msg_q);
    update_msg_status_s_ = conn_.prepare(db::update_msg_status_q);
    update_msg_status_error_s_ = conn_.prepare(db::update_msg_status_error_q);
    update_msg_pid_s_ = conn_.prepare(db::update_msg_pid_q);
  }

  void close_statements__()
  {
    DMITIGR_ASSERT(conn_);
    update_msg_pid_s_.close();
    update_msg_status_error_s_.close();
    update_msg_status_s_.close();
    select_msg_s_.close();
    insert_msg_s_.close();
  }
};

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_SQLITE_CONVEYOR_HPP
