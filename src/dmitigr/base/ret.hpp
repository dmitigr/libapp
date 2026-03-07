// -*- C++ -*-
//
// Copyright 2026 Dmitry Igrishin
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

#ifndef DMITIGR_BASE_RET_HPP
#define DMITIGR_BASE_RET_HPP

#include "concepts.hpp"
#include "error.hpp"

#include <system_error>
#include <type_traits>
#include <utility>

namespace dmitigr {

/**
 * @brief A function return value.
 *
 * @details This type is useful for returning instead of throwing an exception.
 */
template<typename T, Pure_type E = std::error_code>
struct Ret final {
  /// Denotes `void`.
  struct Nothing final {};

  /// The alias of the error type.
  using Error = E;

  /// The alias of the result type.
  using Result = std::conditional_t<std::is_void_v<T>, Nothing, T>;

  /// The result and error types must not be the same.
  static_assert(!std::is_same_v<std::decay_t<Result>, Error>);

  /// Holds not an error and a default-constructed value of type Result.
  Ret() noexcept = default;

  /// Holds not an error and a given `result`.
  Ret(Result&& result) noexcept
    : res{std::move(result)}
  {}

  /// Holds an error and a default-constructed value of type Result.
  Ret(Error&& e) noexcept
    : err{std::move(e)}
  {}

  /// @overload
  template<typename ErrCodeEnum,
    typename = std::enable_if_t<std::is_error_code_enum_v<ErrCodeEnum>>>
  Ret(const ErrCodeEnum ec) noexcept
    : err{ec}
  {}

  /**
   * @brief Holds the both `error` and `result`.
   *
   * @details Useful to return both an error and a partial result.
   */
  Ret(Error&& error, Result&& result) noexcept
    : err{std::move(error)}
    , res{std::move(result)}
  {}

  /// @returns `true` if this instance is not an error.
  explicit operator bool() const noexcept
  {
    return !err;
  }

  /// @returns `true` if the type of result is actually `void`.
  constexpr bool is_void() const noexcept
  {
    return std::is_same_v<Result, Nothing>;
  }

  Error err;
  Result res{};
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_RET_HPP
