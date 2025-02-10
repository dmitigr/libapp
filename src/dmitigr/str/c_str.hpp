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

#ifndef DMITIGR_STR_C_STR_HPP
#define DMITIGR_STR_C_STR_HPP

#include "../base/traits.hpp"

#include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <initializer_list>
#include <type_traits>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// C-strings
// -----------------------------------------------------------------------------

template<typename Ch>
std::size_t len(const Ch* const str) noexcept
{
  if constexpr (std::is_same_v<Ch, char>)
    return std::strlen(str);
  else if constexpr (std::is_same_v<Ch, wchar_t>)
    return std::wcslen(str);
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
inline auto* value_or_empty(const char* const value) noexcept
{
  return value ? value : "";
}

/// @overload
inline auto* value_or_empty(const wchar_t* const value) noexcept
{
  return value ? value : L"";
}

/**
 * @returns The first non-null value of specified `values`, or `nullptr` if all
 * the `values` are nulls.
 */
inline const char* coalesce(std::initializer_list<const char*> values) noexcept
{
  for (const auto value : values) {
    if (value)
      return value;
  }
  return nullptr;
}

/// @overload
inline const wchar_t* coalesce(std::initializer_list<const wchar_t*> values) noexcept
{
  for (const auto value : values) {
    if (value)
      return value;
  }
  return nullptr;
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_C_STR_HPP
