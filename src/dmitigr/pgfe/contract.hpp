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

#ifndef DMITIGR_PGFE_CONTRACT_HPP
#define DMITIGR_PGFE_CONTRACT_HPP

#include "../base/utility.hpp"
#include "types_fwd.hpp"

namespace dmitigr::pgfe::detail {

/**
 * @returns `value`.
 *
 * @throws Generic_exception if `!value`.
 */
template<typename T>
auto forward_or_throw(T&& value, const char* const what)
{
  return dmitigr::forward_or_throw<Generic_exception>(std::forward<T>(value), what);
}

} // namespace dmitigr::pgfe::detail

#endif  // DMITIGR_PGFE_CONTRACT_HPP
