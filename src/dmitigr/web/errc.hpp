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

/**
 * @file
 *
 * @remarks The code of this file is exception-free!
 */

#ifndef DMITIGR_WEB_ERRC_HPP
#define DMITIGR_WEB_ERRC_HPP

namespace dmitigr::web {

/**
 * @ingroup errors
 *
 * @brief Error conditions.
 *
 * @see to_literal(Errc).
 */
enum class Errc {
  /// Service is not ready.
  service_not_ready = 10011,

  /// Lisp expression is not a template.
  lisp_expr_not_tpl = 20011,
  /// Lisp expression is not a template stack.
  lisp_expr_not_tplstack = 20021,

  /// File not found.
  file_not_found = 30011,

  /// Template cyclicity detected.
  tpl_cycle = 40111,

  /// Text is invalid.
  txt_invalid = 50011
};

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 *
 * @see Errc, to_literal_anyway(Errc).
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::service_not_ready:
    return "service_not_ready";

  case Errc::lisp_expr_not_tpl:
    return "lisp_expr_not_tpl";
  case Errc::lisp_expr_not_tplstack:
    return "lisp_expr_not_tplstack";

  case Errc::file_not_found:
    return "file_not_found";

  case Errc::tpl_cycle:
    return "tpl_cycle";

  case Errc::txt_invalid:
    return "txt_invalid";
  }
  return nullptr;
}

/**
 * @returns The literal returned by `to_literal(errc)`, or literal
 * `unknown error` if `to_literal(errc)` returned `nullptr`.
 *
 * @see to_literal(Errc).
 */
constexpr const char* to_literal_anyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_ERRC_HPP
