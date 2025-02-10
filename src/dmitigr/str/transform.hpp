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

#ifndef DMITIGR_STR_TRANSFORM_HPP
#define DMITIGR_STR_TRANSFORM_HPP

#include "../base/assert.hpp"
#include "basics.hpp"
#include "predicate.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// Transformators
// -----------------------------------------------------------------------------

/// @returns The string with the specified `delimiter` between the characters.
inline std::string sparsed_string(const std::string_view input,
  const Byte_format result_format, std::string_view delimiter = "")
{
  if (!input.data() || input.empty())
    return std::string{};
  else if (!delimiter.data())
    delimiter = "";

  std::string result;

  // Go fast path if `raw`.
  if (result_format == Byte_format::raw) {
    if (delimiter.empty())
      return std::string{input};

    result.reserve(input.size() + (input.size() - 1) * delimiter.size());
    auto i = cbegin(input);
    auto const e = cend(input) - 1;
    for (; i != e; ++i) {
      result += *i;
      result += delimiter;
    }
    result += *i;
    return result;
  }

  // Go generic path.
  const auto [elem_sz, fmt_str] = [result_format]
  {
    switch (result_format) {
    case Byte_format::raw: return std::make_pair(1, "%c");
    case Byte_format::hex: return std::make_pair(2, "%02x");
    }
    throw std::invalid_argument{"unsupported result format for"
      " dmitigr::str::to_string(string_view, Byte_format, string_view)"};
  }();

  result.resize(
    input.size()*elem_sz + // for bytes in result_format
    1 + // for `\0` written by std::snprintf()
    input.size()*delimiter.size() // for delimiters
                );
  for (std::string_view::size_type i{}; i < input.size(); ++i) {
    const auto res = result.data() + elem_sz*i + delimiter.size()*i;
    DMITIGR_ASSERT(res - result.data() + elem_sz + delimiter.size()
      <= result.size());
    const int count = std::snprintf(res, elem_sz + 1, fmt_str,
      static_cast<unsigned char>(input[i]));
    DMITIGR_ASSERT(count == elem_sz);
    std::strncpy(res + count, delimiter.data(), delimiter.size());
  }
  result.resize(result.size() - 1 - delimiter.size());
  return result;
}

/// Eliminates duplicate characters from string `str`.
inline void eliminate_duplicates(std::string& str)
{
  auto new_size = str.size();
  for (decltype(new_size) i{}; i < new_size; ++i) {
    const char ch = str[i];
    const auto b = begin(str) + i + 1;
    const auto e = begin(str) + new_size;
    if (const auto it = find(b, e, ch); it != e) {
      (void)remove_if(it, e, [ch, &new_size](const char c)
      {
        if (c == ch) {
          --new_size;
          return true;
        } else
          return false;
      });
    }
  }
  str.resize(new_size);
}

/**
 * @brief Trims `str`.
 *
 * @param str The string to operate on.
 * @param tr Trimming mode.
 * @param predicate The callable with signature `bool(int)`, which is applied
 * to each character of `str` and returns `true` to indicate the character to
 * trim.
 */
template<typename Predicate>
void trim(std::string& str, const Trim tr, const Predicate& predicate)
{
  if (str.empty())
    return;

  const auto b = begin(str);
  const auto e = end(str);
  const auto tb = static_cast<bool>(tr & Trim::lhs) ?
    find_if_not(b, e, predicate) : b;
  if (tb == e) {
    // The string consists of characters to trim, so just clear it out.
    str.clear();
    return;
  }
  const auto te = static_cast<bool>(tr & Trim::rhs) ?
    find_if_not(rbegin(str), rend(str), predicate).base() : e;

  const std::string::size_type new_size = te - tb;
  if (new_size != str.size()) {
    if (tb != b)
      move(tb, te, b);
    str.resize(new_size);
  }
}

/// @overload
inline void trim(std::string& str, const Trim tr = Trim::all)
{
  trim(str, tr, is_not_visible);
}

/// @returns The result of call `trim(str, tr, predicate)`.
template<typename Predicate>
std::string trimmed(std::string str, const Trim tr, const Predicate& predicate)
{
  trim(str, tr, predicate);
  return str;
}

/// @overload
inline std::string trimmed(std::string str, const Trim tr = Trim::all)
{
  return trimmed(str, tr, is_not_visible);
}

/// @overload
template<typename CharT, class Traits, typename Predicate>
std::basic_string_view<CharT, Traits>
trimmed(const std::basic_string_view<CharT, Traits> str,
  const Trim tr, const Predicate& predicate) noexcept
{
  if (str.empty())
    return str;

  const auto b = cbegin(str);
  const auto e = cend(str);
  const auto tb = static_cast<bool>(tr & Trim::lhs) ?
    std::find_if_not(b, e, predicate) : b;
  if (tb == e) {
    // The string consists of characters to trim, so just return empty view.
    return str.substr(0, 0);
  }
  const auto te = static_cast<bool>(tr & Trim::rhs) ?
    std::find_if_not(rbegin(str), rend(str), predicate).base() : e;
  const auto new_size = te - tb;
  return str.substr(tb - b, new_size);
}

/// @overload
template<typename CharT, class Traits>
std::basic_string_view<CharT, Traits>
trimmed(const std::basic_string_view<CharT, Traits> str,
  const Trim tr = Trim::all) noexcept
{
  return trimmed(str, tr, is_not_visible);
}

/// @returns Vector of string pointers.
template<typename String>
auto vector_c_str(const std::vector<String>& args)
{
  std::vector<const typename String::value_type*> argv(args.size() + 1, nullptr);
  std::transform(args.cbegin(), args.cend(), argv.begin(),
    [](const String& arg){return arg.data();});
  return argv;
}

// -----------------------------------------------------------------------------
// lowercase
// -----------------------------------------------------------------------------

/**
 * @brief Replaces all of uppercase characters in `str` by the corresponding
 * lowercase characters.
 */
inline void lowercase(std::string& str)
{
  auto b = begin(str);
  auto e = end(str);
  transform(b, e, b, [](const auto c){return tolower(c);});
}

/**
 * @returns The modified copy of the `str` with all of uppercase characters
 * replaced by the corresponding lowercase characters.
 */
inline std::string to_lowercase(std::string result)
{
  lowercase(result);
  return result;
}

/// @returns `true` if all of characters of `str` are in uppercase.
inline bool is_lowercased(const std::string_view str) noexcept
{
  return std::all_of(cbegin(str), cend(str), [](const auto c)
  {
    return islower(c);
  });
}

// -----------------------------------------------------------------------------
// uppercase
// -----------------------------------------------------------------------------

/**
 * @brief Replaces all of lowercase characters in `str` by the corresponding
 * uppercase characters.
 */
inline void uppercase(std::string& str)
{
  auto b = begin(str);
  auto e = end(str);
  transform(b, e, b, [](const auto c){return toupper(c);});
}

/**
 * @returns The modified copy of the `str` with all of lowercase characters
 * replaced by the corresponding uppercase characters.
 */
inline std::string to_uppercase(std::string result)
{
  uppercase(result);
  return result;
}

/// @returns `true` if all of character of `str` are in lowercase.
inline bool is_uppercased(const std::string_view str) noexcept
{
  return std::all_of(cbegin(str), cend(str), [](const auto c)
  {
    return isupper(c);
  });
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_TRANSFORM_HPP
