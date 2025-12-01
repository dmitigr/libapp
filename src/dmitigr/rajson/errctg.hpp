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

#ifndef DMITIGR_RAJSON_ERRCTG_HPP
#define DMITIGR_RAJSON_ERRCTG_HPP

#include "../base/error.hpp"
#include "rapidjson.hpp"

#include "errc.hpp"

#include <string>
#include <system_error>

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_code_enum<rapidjson::ParseErrorCode> final : true_type {};

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_code_enum<dmitigr::rajson::Errc> final : true_type {};

} // namespace std

namespace dmitigr::rajson {

// -----------------------------------------------------------------------------
// Parse error category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The parse error category.
 *
 * @see Parse_exception.
 */
class Parse_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_rajson_parse_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_rajson_parse_error";
  }

  /**
   * @returns The string that describes the error code denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of `rapidjson::ParseErrorCode`.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    std::string result{name()};
    result += ' ';
    result += std::to_string(ev);
    if (const auto* const literal =
      rapidjson::GetParseError_En(static_cast<rapidjson::ParseErrorCode>(ev))) {
      result += ' ';
      result += literal;
    }
    return result;
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Parse_error_category.
 */
inline const Parse_error_category& parse_error_category() noexcept
{
  static const Parse_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), parse_error_category())`.
 */
inline std::error_code
make_error_code(const rapidjson::ParseErrorCode errc) noexcept
{
  return std::error_code{static_cast<int>(errc), parse_error_category()};
}

// -----------------------------------------------------------------------------
// Generic error category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief A generic category of errors.
 *
 * @see Exception.
 */
class Generic_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_rajson_generic_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_rajson_generic_error";
  }

  /**
   * @returns The string that describes the error code denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    const char* const desc{to_literal_anyway(static_cast<Errc>(ev))};
    constexpr const char* const sep{": "};
    std::string result;
    result.reserve(std::strlen(name()) + std::strlen(sep) + std::strlen(desc));
    return result.append(name()).append(sep).append(desc);
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Generic_error_category.
 */
inline const Generic_error_category& generic_error_category() noexcept
{
  static const Generic_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), generic_error_category())`.
 */
inline std::error_code make_error_code(const Errc errc) noexcept
{
  return {static_cast<int>(errc), generic_error_category()};
}

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_ERRCTG_HPP
