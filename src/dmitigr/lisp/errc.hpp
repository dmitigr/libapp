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

#ifndef DMITIGR_LISP_ERRC_HPP
#define DMITIGR_LISP_ERRC_HPP

#include "../base/error.hpp"

namespace dmitigr::lisp {

/**
 * @ingroup errors
 *
 * @brief Error codes.
 */
enum class Errc {
  /// Not an error.
  ok = 0,

  /// Generic error.
  generic = 1,

  /// Expression is empty.
  parse_empty = 10011,
  /// Expression is malformed.
  parse_malformed = 10021,
  /// Expression is incomplete.
  parse_incomplete = 10031,
  /// Expression contains invalid name of variable.
  parse_var_invalid_name = 10111,
  /// Expression contains invalid name of function.
  parse_fun_invalid_name = 10211,
  /// Expression contains invalid name of special.
  parse_spec_invalid_name = 10311,
  /// Expression contains invalid number.
  parse_num_invalid = 10411,

  /// Expression is not a local variable.
  expr_not_lvar = 20011,
  /// Expression is not a global variable.
  expr_not_gvar = 20021,
  /// Expression is not a function.
  expr_not_function = 20031,
  /// Expression is not a error.
  expr_not_error = 20041,
  /// Expression is not a boolean.
  expr_not_boolean = 20051,
  /// Expression is not a number.
  expr_not_number = 20061,
  /// Expression is not a string.
  expr_not_string = 20071,
  /// Expression is not a tuple.
  expr_not_tuple = 20081,
  /// Expression is not comparable.
  expr_undefined_cmp = 21011,

  /// Function handler is unknown (not registered).
  fun_unknown = 30011,
  /// Function usage is not correct.
  fun_usage = 30021,

  /// Variable is unbound.
  var_unbound = 40011,

  /// Attempt to divide a number by zero.
  num_division_by_zero = 50011,

  /// Unhandled break (`break` called not in loop).
  unhandled_break = 60011,
  /// Unhandled end (`end` called not in `begin`).
  unhandled_end = 60021
};

/**
 * @ingroup errors
 *
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::ok:
    return "ok";

  case Errc::generic:
    return "generic";

  case Errc::parse_empty:
    return "parse_empty";
  case Errc::parse_malformed:
    return "parse_malformed";
  case Errc::parse_incomplete:
    return "parse_incomplete";
  case Errc::parse_var_invalid_name:
    return "parse_var_invalid_name";
  case Errc::parse_fun_invalid_name:
    return "parse_fun_invalid_name";
  case Errc::parse_spec_invalid_name:
    return "parse_spec_invalid_name";
  case Errc::parse_num_invalid:
    return "parse_num_invalid";

  case Errc::expr_not_lvar:
    return "expr_not_lvar";
  case Errc::expr_not_gvar:
    return "expr_not_gvar";
  case Errc::expr_not_function:
    return "expr_not_function";
  case Errc::expr_not_error:
    return "expr_not_error";
  case Errc::expr_not_boolean:
    return "expr_not_boolean";
  case Errc::expr_not_number:
    return "expr_not_number";
  case Errc::expr_not_string:
    return "expr_not_string";
  case Errc::expr_not_tuple:
    return "expr_not_tuple";

  case Errc::expr_undefined_cmp:
    return "expr_undefined_cmp";

  case Errc::fun_unknown:
    return "fun_unknown";
  case Errc::fun_usage:
    return "fun_usage";

  case Errc::var_unbound:
    return "var_unbound";

  case Errc::num_division_by_zero:
    return "num_divizion_by_zero";

  case Errc::unhandled_break:
    return "unhandled_break";
  case Errc::unhandled_end:
    return "unhandled_end";
  }
  return nullptr;
}

/**
 * @ingroup errors
 *
 * @returns The literal returned by `to_literal(errc)`, or literal which
 * denotes "unknown error" if `to_literal(errc)` returned `nullptr`.
 */
constexpr const char* to_literal_anyway(const Errc errc) noexcept
{
  return dmitigr::to_literal_anyway(errc);
}

} // namespace dmitigr::lisp

#endif  // DMITIGR_LISP_ERRC_HPP
