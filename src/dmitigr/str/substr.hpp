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

#ifndef DMITIGR_STR_SUBSTR_HPP
#define DMITIGR_STR_SUBSTR_HPP

#include "../base/traits.hpp"
#include "basics.hpp"
#include "predicate.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// Substrings
// -----------------------------------------------------------------------------

/**
 * @returns The position of the first non-space character of `str` in the range
 * [pos, str.size()), or `std::string_view::npos` if there is no such a position.
 *
 * @par Requires
 * `pos <= str.size()`.
 */
inline std::string_view::size_type
first_non_space_pos(const std::string_view str, const std::string_view::size_type pos)
{
  if (!(pos <= str.size()))
    throw Exception{"cannot get position of non space by using invalid offset"};

  const auto b = cbegin(str);
  const auto e = cend(str);
  const auto i = std::find_if(b + pos, e, is_not_space);
  return i != e ? static_cast<std::string_view::size_type>(i - b)
    : std::string_view::npos;
}

// =============================================================================
// for_each_part()
// =============================================================================

template<Fepsep_type T, typename CharT, class TraitsT = std::char_traits<CharT>>
struct Fepsep {
  constexpr static Fepsep_type Type = T;
  using String_view = std::basic_string_view<CharT, TraitsT>;

  String_view str;

  Fepsep() = default;

  explicit Fepsep(const CharT* const s)
    : Fepsep{String_view{s}}
  {}

  explicit Fepsep(String_view s)
    : str{s}
  {}
};

template<typename C, class T = std::char_traits<C>>
using Fepsep_all = Fepsep<Fepsep_type::all, C, T>;

template<typename C, class T = std::char_traits<C>>
using Fepsep_any = Fepsep<Fepsep_type::any, C, T>;

template<typename C, class T = std::char_traits<C>>
using Fepsep_none = Fepsep<Fepsep_type::none, C, T>;

template<bool IsForward = true,
  typename F, Fepsep_type Type, typename CharT, class Traits>
void for_each_part(F&& callback,
  const std::basic_string_view<CharT, Traits> str,
  const Fepsep<Type, CharT, Traits> sep)
{
  using String_view = std::basic_string_view<CharT, Traits>;
  using Size = typename String_view::size_type;

  const auto str_sz = str.size();
  const auto sep_sz = sep.str.size();

  if (!str_sz)
    return;
  else if (!sep_sz)
    throw std::invalid_argument{"invalid separtor for dmitigr::str::for_each_part"};

  constexpr auto is_all = Type == Fepsep_type::all;
  constexpr auto is_not = Type == Fepsep_type::none;
  const auto in_sep = [b = sep.str.cbegin(), e = sep.str.cend()](const auto ch) noexcept
  {
    return std::any_of(b, e, [c = ch](const auto ch) noexcept
    {
      return c == ch;
    });
  };

  const auto find_sep = [str, sep = sep.str](const auto offset)
  {
    if constexpr (Type == Fepsep_type::all)
      if constexpr (IsForward)
        return str.find(sep, offset);
      else
        return str.rfind(sep, offset);
    else if constexpr (Type == Fepsep_type::any)
      if constexpr (IsForward)
        return str.find_first_of(sep, offset);
      else
        return str.find_last_of(sep, offset);
    else if constexpr (Type == Fepsep_type::none)
      if constexpr (IsForward)
        return str.find_first_not_of(sep, offset);
      else
        return str.find_last_not_of(sep, offset);
    else
      static_assert(false_value<CharT>, "invalid separator type");
  };

  Size offset = IsForward ? 0 : str.empty() ? 0 : str.size() - 1;
  while (true) {
    auto sep_pos = find_sep(offset);
    const auto count = sep_pos != String_view::npos ?
      (IsForward ? sep_pos - offset : offset + 1 - sep_pos - (is_all ? sep_sz : 1)) :
      (IsForward ? String_view::npos : offset + 1);
    const auto substr_offset = IsForward ? offset :
      sep_pos != String_view::npos ? sep_pos + (is_all ? sep_sz : 1) : 0;
    const auto result = str.substr(substr_offset, count);
    if (!callback(result))
      break;

    if (sep_pos == String_view::npos)
      break;

    if constexpr (IsForward) {
      if constexpr (is_all) {
        offset = sep_pos + sep.str.size();
      } else {
        for (offset = sep_pos + 1; offset < str_sz; ++offset) {
          if (is_not ^ !in_sep(str[offset]))
            break;
        }
      }
      if (!(offset < str_sz))
        break;
    } else {
      if constexpr (is_all) {
        if (sep_pos >= 1)
          offset = sep_pos - 1;
        else
          break;
      } else {
        if (!(sep_pos >= 1))
          break;
        for (offset = sep_pos - 1;; --offset) {
          if (offset == 0 || is_not ^ !in_sep(str[offset]))
            break;
        }
      }
    }
  }
}

template<bool IsForward = true, typename F, typename CharT, class S>
void for_each_part(F&& callback, const CharT* const str, S&& sep)
{
  for_each_part<IsForward>(std::forward<F>(callback),
    std::basic_string_view{str}, std::forward<S>(sep));
}

template<typename ... Types>
void for_each_part_reverse(Types&& ... args)
{
  for_each_part<false>(std::forward<Types>(args)...);
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_SUBSTR_HPP
