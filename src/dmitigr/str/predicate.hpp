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

#ifndef DMITIGR_STR_PREDICATE_HPP
#define DMITIGR_STR_PREDICATE_HPP

#include <algorithm>
#include <cctype>
#include <string_view>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// Predicates
// -----------------------------------------------------------------------------

/// @returns `true` if `c` is a valid space character.
inline bool is_space(const int ch) noexcept
{
  return std::isspace(ch);
}

/// @returns `!is_space(ch)`.
inline bool is_not_space(const int ch) noexcept
{
  return !is_space(ch);
}

/// @returns `true` if `c` is printable character.
inline bool is_printable(const int ch) noexcept
{
  return std::isprint(ch);
}

/// @returns `!is_printable(ch)`.
inline bool is_not_printable(const int ch) noexcept
{
  return !is_printable(ch);
}

/// @returns `true` if `c` is printable character and not space.
inline bool is_visible(const int ch) noexcept
{
  return std::isprint(ch) && is_not_space(ch);
}

/// @returns `!is_visible(ch)`.
inline bool is_not_visible(const int ch) noexcept
{
  return !is_visible(ch);
}

/// @returns `true` if `c` is a zero character.
inline bool is_zero(const int ch) noexcept
{
  return !ch;
}

/// @returns `true` if `c` is a not zero character.
inline bool is_not_zero(const int ch) noexcept
{
  return !is_zero(ch);
}

/// @returns `true` if `str` is a blank or empty string.
inline bool is_blank(const std::string_view str) noexcept
{
  return std::all_of(cbegin(str), cend(str), is_space);
}

/// @returns `true` if `str` has at least one space character.
inline bool has_space(const std::string_view str) noexcept
{
  return std::any_of(cbegin(str), cend(str), is_space);
}

/// @returns `true` if `input` is starting with `pattern`.
inline bool is_begins_with(const std::string_view input,
  const std::string_view pattern) noexcept
{
  return (pattern.size() <= input.size()) &&
    std::equal(cbegin(pattern), cend(pattern), cbegin(input));
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_PREDICATE_HPP
