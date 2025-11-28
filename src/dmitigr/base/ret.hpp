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

#ifndef DMITIGR_BASE_RET_HPP
#define DMITIGR_BASE_RET_HPP

#include "error.hpp"

#include <system_error>
#include <type_traits>
#include <utility>

namespace dmitigr {

struct Nothing final {
  using Type = void;
};

template<typename T>
using Ret_result = std::conditional_t<std::is_void_v<T>, Nothing, T>;

/**
 * @brief A function return value.
 *
 * @details This template struct is useful as the return type of functions which
 * must not throw exceptions.
 */
template<typename T>
struct Ret final {
  /// The alias of the error type.
  using Error = Err;

  /// The alias of the result type.
  using Result = Ret_result<T>;

  static_assert(!std::is_same_v<std::decay_t<Result>, Error>);

  /// Holds not an error and a default-constructed value of type Result.
  Ret() noexcept = default;

  /// Holds an error and a default-constructed value of type Result.
  Ret(Error e) noexcept
    : err{std::move(e)}
  {}

  /// @overload
  template<typename ErrCodeEnum,
    typename = std::enable_if_t<std::is_error_code_enum_v<ErrCodeEnum>>>
  Ret(const ErrCodeEnum ec) noexcept
    : err{ec}
  {}

  /// Holds not an error and a given value of type T.
  Ret(Result r) noexcept
    : res{std::move(r)}
  {}

  /**
   * @brief Holds the both `err` and `res`.
   *
   * @details This constructor is useful to return an error with an information
   * provided by `res`.
   */
  Ret(Error e, Result r) noexcept
    : err{e}
    , res{std::move(r)}
  {}

  /// @returns The error.
  template<typename E, typename ... Types>
  static auto make_error(E e, Types&& ... res_args) noexcept
  {
    return Ret{Error{std::move(e)}, Result{std::forward<Types>(res_args)...}};
  }

  /// @returns The result.
  template<typename ... Types>
  static auto make_result(Types&& ... res_args) noexcept
  {
    return Ret{Result{std::forward<Types>(res_args)...}};
  }

  /// @returns `true` if this instance is not an error.
  explicit operator bool() const noexcept
  {
    return !err;
  }

  Err err;
  T res{};
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_RET_HPP
