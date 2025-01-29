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

#ifndef DMITIGR_JRPC_ERRCTG_HPP
#define DMITIGR_JRPC_ERRCTG_HPP

#include "errc.hpp"

#include <string>
#include <system_error>

namespace dmitigr::jrpc {

// -----------------------------------------------------------------------------
// Server_error_category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The category of runtime server errors.
 *
 * @see Server_error.
 */
class Server_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_jrpc_server_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_jrpc_server_error";
  }

  /**
   * @returns The string that describes the error condition denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Server_errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    std::string result(name());
    result += ' ';
    result += std::to_string(ev);
    result += ' ';
    if (const char* const literal = to_literal(static_cast<Server_errc>(ev))) {
      result += ' ';
      result += literal;
    }
    return result;
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Server_error_category.
 */
inline const Server_error_category& server_error_category() noexcept
{
  static const Server_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), server_error_category())`
 */
inline std::error_condition make_error_condition(const Server_errc errc) noexcept
{
  return std::error_condition{static_cast<int>(errc), server_error_category()};
}

} // namespace dmitigr::jrpc

#endif  // DMITIGR_JRPC_ERRCTG_HPP
