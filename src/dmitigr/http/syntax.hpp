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

#ifndef DMITIGR_HTTP_SYNTAX_HPP
#define DMITIGR_HTTP_SYNTAX_HPP

#include <algorithm>
#include <cctype>
#include <string>

namespace dmitigr::http {
namespace detail {

constexpr bool is_ctl(const char c) noexcept
{
  return (0 <= c && c <= 31) || (c == 127);
}

namespace rfc6265 {

/**
 * @internal
 *
 * @returns `true` if `c` is a valid token character according to
 * https://tools.ietf.org/html/rfc6265#section-4.1.1, or `false` otherwise.
 */
inline bool is_valid_token_character(const unsigned char c) noexcept
{
  constexpr const unsigned char separators[] =
    {'(', ')', '<', '>', '@', ',', ';', ':', '\\',
     '"', '/', '[', ']', '?', '=', '{', '}', ' ', '\t'};
  return std::isalnum(c) || c == '_' || c == '-' ||
    (!is_ctl(c) && std::none_of(std::cbegin(separators),
      std::cend(separators), [c](const auto ch){return c == ch;}));
}

/**
 * @internal
 *
 * @returns `true` if `c` is a valid cookie octet according to
 * https://tools.ietf.org/html/rfc6265#section-4.1.1, or `false` otherwise.
 */
inline bool is_valid_cookie_octet(const unsigned char c) noexcept
{
  constexpr unsigned char forbidden[] = {'"', ',', ';', '\\'};
  return std::isalnum(c) ||
    (!is_ctl(c) && !std::isspace(c) &&
    std::none_of(std::cbegin(forbidden), std::cend(forbidden),
      [c](const auto ch){return c == ch;}));
}

} // namespace rfc6265
} // namespace detail

/**
 * @ingroup headers
 *
 * @returns `true` if the specified `name` is a valid cookie name, or
 * `false` otherwise.
 */
inline bool is_valid_cookie_name(const std::string_view name) noexcept
{
  return !name.empty() &&
    std::all_of(cbegin(name), cend(name), [](const char ch)
    {
      return detail::rfc6265::is_valid_token_character(ch);
    });
}

/**
 * @ingroup headers
 *
 * @returns `true` if the specified `value` is a valid cookie value, or
 * `false` otherwise.
 */
inline bool is_valid_cookie_value(const std::string_view value) noexcept
{
  return std::all_of(cbegin(value), cend(value), [](const char ch)
  {
    return detail::rfc6265::is_valid_cookie_octet(ch);
  });
}

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_SYNTAX_HPP
