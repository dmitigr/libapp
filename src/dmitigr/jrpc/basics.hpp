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

#ifndef DMITIGR_JRPC_BASICS_HPP
#define DMITIGR_JRPC_BASICS_HPP

namespace dmitigr::jrpc {

/// Represents null.
struct Null final {};

/// A constant of type `Null` that is used to indicate null state.
inline constexpr Null null;

/// Parameters notation.
enum class Parameters_notation {
  /// Positional notation.
  positional,

  /// Named notation.
  named
};

/**
 * @returns The literal representation of the `value`, or `nullptr` if
 * `value` does not corresponds to any value defined by Parameters_notation.
 */
constexpr const char* to_literal(const Parameters_notation value)
{
  switch (value) {
  case Parameters_notation::positional:
    return "positional";
  case Parameters_notation::named:
    return "named";
  }
  return nullptr;
}

} // namespace dmitigr::jrpc

#endif  // DMITIGR_JRPC_BASICS_HPP
