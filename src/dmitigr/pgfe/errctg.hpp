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

#ifndef DMITIGR_PGFE_ERRCTG_HPP
#define DMITIGR_PGFE_ERRCTG_HPP

#include "dll.hpp"
#include "errc.hpp"

#include <system_error>

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for the integration with `<system_error>`.
 */
template<>
struct is_error_code_enum<dmitigr::pgfe::Errc> final : true_type {};

/**
 * @ingroup errors
 *
 * @brief The full specialization for the integration with `<system_error>`.
 */
template<>
struct is_error_code_enum<dmitigr::pgfe::Sqlstate> final : true_type {};

} // namespace std

namespace dmitigr::pgfe {

/**
 * @ingroup errors
 *
 * @brief A category of runtime generic errors.
 *
 * @see Generic_exception.
 */
class Generic_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_pgfe_generic_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_pgfe_generic_error";
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
  DMITIGR_PGFE_API std::string message(int ev) const override;
};

/**
 * @ingroup errors
 *
 * @brief A category of runtime server errors.
 *
 * @see Sqlstate_exception.
 */
class Sqlstate_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_pgfe_sqlstate_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_pgfe_sqlstate_error";
  }

  /**
   * @returns The string that describes the error code denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Sqlstate.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  DMITIGR_PGFE_API std::string message(int ev) const override;
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
 * @returns The reference to the instance of type Sqlstate_error_category.
 */
inline const Sqlstate_error_category& sqlstate_error_category() noexcept
{
  static const Sqlstate_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), generic_error_category())`
 */
inline std::error_code make_error_code(const Errc errc) noexcept
{
  return std::error_code{static_cast<int>(errc), generic_error_category()};
}

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), sqlstate_error_category())`
 */
inline std::error_code make_error_code(const Sqlstate errc) noexcept
{
  return std::error_code{static_cast<int>(errc), sqlstate_error_category()};
}

} // namespace dmitigr::pgfe

#ifndef DMITIGR_PGFE_NOT_HEADER_ONLY
#include "errctg.cpp"
#endif

#endif  // DMITIGR_PGFE_ERRCTG_HPP
