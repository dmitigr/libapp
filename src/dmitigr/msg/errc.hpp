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

#ifndef DMITIGR_MSG_ERRC_HPP
#define DMITIGR_MSG_ERRC_HPP

namespace dmitigr::msg {

/**
 * @ingroup errors
 *
 * @brief Generic error codes (or conditions).
 */
enum class Errc {
  /// Generic error.
  generic = 1,

  /// Message sender is invalid.
  message_sender_invalid = 10011,
  /// Message recipients is invalid.
  message_recipients_invalid = 10111,
  /// Message subject is invalid.
  message_subject_invalid = 10211,
  /// Message content is invalid.
  message_content_invalid = 10311
};

/**
 * @ingroup errors
 *
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::generic:
    return "generic";

  case Errc::message_sender_invalid:
    return "message_sender_invalid";
  case Errc::message_recipients_invalid:
    return "message_recipients_invalid";
  case Errc::message_subject_invalid:
    return "message_subject_invalid";
  case Errc::message_content_invalid:
    return "message_content_invalid";
  }
  return nullptr;
}

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_ERRC_HPP
