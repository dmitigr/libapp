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

#ifndef DMITIGR_MSG_TYPES_FWD_HPP
#define DMITIGR_MSG_TYPES_FWD_HPP

/// The API.
namespace dmitigr::msg {

enum class Errc;
class Generic_error_category;
class Exception;

enum class Msg_status;
struct Message;

class Conveyor;
class Sqlite_conveyor;

class Processor;
class Mail_processor;

/// The implementation details.
namespace detail {
} // namespace detail

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_TYPES_FWD_HPP
