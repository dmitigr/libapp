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

#ifndef DMITIGR_JRPC_ERRC_HPP
#define DMITIGR_JRPC_ERRC_HPP

#include <system_error>

namespace dmitigr::jrpc {

// -----------------------------------------------------------------------------
// Server_errc
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief JSON-RPC server-error codes.
 */
enum class Server_errc {
  /// Invalid JSON was received by the server.
  parse_error = -32700,

  /// The JSON sent is not a valid Request object.
  invalid_request = -32600,

  /// The method does not exist / is not available.
  method_not_found = -32601,

  /// Invalid method parameter(s).
  invalid_params = -32602,

  /// Internal JSON-RPC error.
  internal_error = -32603,

  /// Server error.
  server_error = -32000
};

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Server_errc.
 */
constexpr const char* to_literal(const Server_errc errc) noexcept
{
  switch (errc) {
  case Server_errc::parse_error:
    return "parse_error";
  case Server_errc::invalid_request:
    return "invalid_request";
  case Server_errc::method_not_found:
    return "method_not_found";
  case Server_errc::invalid_params:
    return "invalid_params";
  case Server_errc::internal_error:
    return "internal_error";
  case Server_errc::server_error:
    return "server_error";
  }
  return nullptr;
}

/**
 * @ingroup errors
 *
 * @returns The literal returned by `to_literal(errc)`, or literal
 * "unknown error" if `to_literal(errc)` returned `nullptr`.
 */
template<class E>
constexpr const char* to_literal_anyway(const E errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace dmitigr::jrpc

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_condition_enum<dmitigr::jrpc::Server_errc> final : true_type {};

} // namespace std

#endif  // DMITIGR_JRPC_ERRC_HPP
