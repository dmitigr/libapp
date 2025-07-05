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

#ifndef DMITIGR_RAJSON_ERRC_HPP
#define DMITIGR_RAJSON_ERRC_HPP

namespace dmitigr::rajson {

/**
 * @ingroup errors
 *
 * @brief Generic error codes (or conditions).
 */
enum class Errc {
  /// Generic error.
  generic = 1,

  /// Value is not an object.
  value_not_object = 10011
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

  case Errc::value_not_object:
    return "value_not_object";
  }
  return nullptr;
}

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_ERRC_HPP
