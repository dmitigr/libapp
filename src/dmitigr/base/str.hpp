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

#ifndef DMITIGR_BASE_STR_HPP
#define DMITIGR_BASE_STR_HPP

#include "assert.hpp"
#include "enum.hpp"
#include "exceptions.hpp"
#include "traits.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr {
namespace str {

// =============================================================================
// Basics
// =============================================================================

/// A trimming mode bitmask.
enum class Trim {
  lhs = 0x1,
  rhs = 0x2,
  all = lhs | rhs
};

/// A byte format.
enum class Byte_format {
  raw = 1,
  hex
};

/// A for_each_part() separator type.
enum class Fepsep_type {
  all,
  any,
  none
};

} // namespace str

template<> struct Is_bitmask_enum<str::Trim> : std::true_type {};

} // namespace dmitigr

namespace dmitigr::str {

// =============================================================================
// C-strings
// =============================================================================

template<typename Ch>
std::size_t length(const Ch* const str) noexcept
{
  if constexpr (std::is_same_v<Ch, char>)
    return std::strlen(str);
  else if constexpr (std::is_same_v<Ch, wchar_t>)
    return std::wcslen(str);
  else
    static_assert(false_value<Ch>);
}

template<typename Ch>
Ch* copy(Ch* dst, const Ch* const src, const std::size_t count) noexcept
{
  if constexpr (std::is_same_v<Ch, char>)
    return std::strncpy(dst, src, count);
  else if constexpr (std::is_same_v<Ch, wchar_t>)
    return std::wcsncpy(dst, src, count);
  else
    static_assert(false_value<Ch>);
}

/**
 * @returns The pointer to a next non-space character, or pointer to the
 * terminating zero character.
 */
inline auto* next_non_space_pointer(const char* p) noexcept
{
  if (p) {
    while (*p && std::isspace(*p))
      ++p;
  }
  return p;
}

/// @overload
inline auto* next_non_space_pointer(const wchar_t* p) noexcept
{
  if (p) {
    while (*p && std::iswspace(*p))
      ++p;
  }
  return p;
}

/// @returns The specified `value` if `(value != nullptr)`, or `""` otherwise.
template<typename Ch>
const Ch* value_or_empty(const Ch* const value) noexcept
{
  if constexpr (std::is_same_v<Ch, char>)
    return value ? value : "";
  else if constexpr (std::is_same_v<Ch, wchar_t>)
    return value ? value : L"";
  else
    static_assert(false_value<Ch>);
}

/**
 * @returns The first non-null value of specified `values`, or `nullptr` if all
 * the `values` are nulls.
 */
template<typename Ch>
const Ch* coalesce(std::initializer_list<const Ch*> values) noexcept
{
  for (const auto value : values) {
    if (value)
      return value;
  }
  return nullptr;
}

// =============================================================================
// Lines
// =============================================================================

/**
 * @returns The line number (which starts at 0) by the given absolute position.
 *
 * @par Requires
 * `(pos < str.size())`.
 */
inline auto
line_number_by_position(const std::string& str, const std::string::size_type pos)
{
  if (!(pos < str.size()))
    throw Exception{"cannot get line number by invalid position"};

  using Diff = decltype(cbegin(str))::difference_type;
  return std::count(cbegin(str), cbegin(str) + static_cast<Diff>(pos), '\n');
}

/**
 * @returns The line and column numbers (both starts at 0) by the given absolute
 * position.
 *
 * @par Requires
 * `(pos < str.size())`.
 */
inline std::pair<std::size_t, std::size_t>
line_column_numbers_by_position(const std::string& str,
  const std::string::size_type pos)
{
  if (!(pos < str.size()))
    throw Exception{"cannot get line and column numbers by invalid position"};

  std::size_t line{};
  std::size_t column{};
  for (std::size_t i = 0; i < pos; ++i) {
    ++column;
    if (str[i] == '\n') {
      ++line;
      column = 0;
    }
  }
  return std::make_pair(line, column);
}

/**
 * @brief Extracts lines from `buffer` to `result`.
 *
 * @param result The destination vector.
 * @param buffer The source buffer.
 * @param is_remove_cr The directive to remove the carriage return character
 * from the end of each extracted line.
 *
 * @par Effects
 * `result` contains the extracted lines appended to the back.
 * `buffer` contains uncompleted line (if any) which is unextracted to `result`.
 *
 * @returns The number of extracted lines.
 */
inline std::string::size_type
get_lines(std::vector<std::string>& result, std::string& buffer,
  const bool is_remove_cr = true)
{
  std::string::size_type found_count{};
  std::string::size_type start_pos{};
  while (true) {
    const auto end_pos = buffer.find('\n', start_pos);
    if (end_pos != std::string::npos) {
      result.emplace_back(buffer.substr(start_pos, end_pos - start_pos));
      if (is_remove_cr && !result.back().empty() && result.back().back() == '\r')
        result.back().pop_back();
      start_pos = end_pos + 1;
      ++found_count;
    } else {
      if (start_pos) {
        if (start_pos < buffer.size())
          buffer = buffer.substr(start_pos);
        else
          buffer.clear();
      }
      break;
    }
  }
  return found_count;
}

// =============================================================================
// Numeric conversions
// =============================================================================

/**
 * @returns The string with the character representation
 * of the `value` according to the given `base`.
 *
 * @par Requires
 * `(2 <= base && base <= 36)`.
 */
template<typename Number>
std::enable_if_t<std::is_integral<Number>::value, std::string>
to_string(Number value, const Number base = 10)
{
  static_assert(std::numeric_limits<Number>::min() <= 2 &&
    std::numeric_limits<Number>::max() >= 36);

  if (!(2 <= base && base <= 36))
    throw Exception{"cannot convert number to text by using invalid base"};

  constexpr const char digits[] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
     'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
     'U', 'V', 'W', 'X', 'Y', 'Z'};
  static_assert(sizeof(digits) == 36);
  const bool negative = (value < 0);
  std::string result;
  if (negative)
    value = -value;
  while (value >= base) {
    const auto rem = value % base;
    value /= base;
    result += digits[rem];
  }
  result += digits[value];
  if (negative)
    result += '-';
  std::reverse(begin(result), end(result));
  return result;
}

// =============================================================================
// Predicates
// =============================================================================

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

// =============================================================================
// Sequence conversions
// =============================================================================

/// @returns The string with stringified elements of the sequence in range `[b, e)`.
template<class InputIterator, typename Function>
std::string to_string(InputIterator b, const InputIterator e,
  const std::string_view sep, const Function& to_str)
{
  std::string result;
  if (b != e) {
    while (true) {
      result.append(to_str(*b));
      ++b;
      if (b != e)
        result.append(sep);
      else
        break;
    }
  }
  return result;
}

/// @returns The string with stringified elements of the `Container`.
template<class Container, typename Function>
std::string to_string(const Container& cont, const std::string_view sep,
  const Function& to_str)
{
  return to_string(cbegin(cont), cend(cont), sep, to_str);
}

/// @returns The string with stringified elements of the `Container`.
template<class Container>
std::string to_string(const Container& cont, const std::string_view sep)
{
  return to_string(cont, sep, [](const std::string& e) -> const auto&
  {
    return e;
  });
}

/**
 * @brief Splits the `input` string into the parts by using the
 * specified `separators`.
 *
 * @param input An input string.
 * @param separators Separators.
 * @param to_type A converter from std::string_view to T.
 *
 * @returns The vector of splitted parts converted to T.
 */
template<class T, typename F>
std::vector<T> to_vector(const std::string_view input,
  const std::string_view separators, const F& to_type)
{
  using Size = std::string_view::size_type;
  std::vector<T> result;
  result.reserve(8);
  Size pos{std::string_view::npos};
  Size offset{};
  while (offset < input.size()) {
    pos = input.find_first_of(separators, offset);
    const auto part_size = std::min<Size>(pos, input.size()) - offset;
    result.push_back(to_type(input.substr(offset, part_size)));
    offset += part_size + 1;
  }
  if (pos != std::string_view::npos) // input ends with a separator
    result.push_back(to_type(std::string_view{}));
  return result;
}

/**
 * @brief Splits the `input` string into the parts separated by the
 * specified `separators`.
 *
 * @returns The vector of splitted parts.
 */
template<class S = std::string>
std::vector<S> to_vector(const std::string_view input,
  const std::string_view separators)
{
  return to_vector<S>(input, separators, [](const auto& v){return S{v};});
}

// =============================================================================
// Substrings
// =============================================================================

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

// -----------------------------------------------------------------------------
// for_each_part()
// -----------------------------------------------------------------------------

/// A for_each_part() (FEP) separator.
template<Fepsep_type T, typename CharT, class TraitsT = std::char_traits<CharT>>
struct Fepsep final {
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

/// An alias for a FEP-separator represented by the exact string specified.
template<typename C, class T = std::char_traits<C>>
using Fepsep_all = Fepsep<Fepsep_type::all, C, T>;

/**
 * An alias for a FEP-separator represented by any character of the
 * specified string.
 */
template<typename C, class T = std::char_traits<C>>
using Fepsep_any = Fepsep<Fepsep_type::any, C, T>;

/**
 * An alias for a FEP-separator represented by none of characters of the
 * specified string.
 */
template<typename C, class T = std::char_traits<C>>
using Fepsep_none = Fepsep<Fepsep_type::none, C, T>;

/**
 * @brief Splits the string `str` into parts according to the separator `sep`.
 *
 * @tparam IsForward Specifies the direction of the string traversal.
 * @param callback The function with signature
 * `bool(std::basic_string_view<CharT, Traits> part)` which is called to each
 * part of `str`.
 * @param str The string to split.
 * @param sep The separator.
 */
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
      static_assert(false_value<CharT>, "invalid FEP-separator type");
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

/// @overload
template<bool IsForward = true, typename F, typename CharT, class S>
void for_each_part(F&& callback, const CharT* const str, S&& sep)
{
  for_each_part<IsForward>(std::forward<F>(callback),
    std::basic_string_view{str}, std::forward<S>(sep));
}

/// A convenient shortcut of for_each_part<false>().
template<typename ... Types>
void for_each_part_backward(Types&& ... args)
{
  for_each_part<false>(std::forward<Types>(args)...);
}

// =============================================================================
// Transformators
// =============================================================================

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

#endif  // DMITIGR_BASE_STR_HPP
