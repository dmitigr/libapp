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

#include "contract.hpp"
#include "errctg.hpp"
#include "error.hpp"
#include "exceptions.hpp"

namespace dmitigr::pgfe {

DMITIGR_PGFE_INLINE Generic_exception::Generic_exception(const Errc errc,
  std::string what)
  : Exception{errc,
    what.empty() ?
    std::string{"dmitigr::pgfe error: "}.append(to_literal(errc)) :
    std::string{"dmitigr::pgfe error: "}.append(what)}
{}

DMITIGR_PGFE_INLINE Generic_exception::Generic_exception(const std::string& what)
  : Generic_exception{Errc::generic, what}
{}

// =============================================================================

DMITIGR_PGFE_INLINE Sqlstate_exception::Sqlstate_exception(std::shared_ptr<Error> error)
  : Exception{detail::forward_or_throw(error, "Sqlstate_exception(error)")->condition(),
      std::string{"PostgreSQL error: "}.append(detail::forward_or_throw(error,
        "Sqlstate_exception(error)")->brief())}
  , error_{std::move(error)}
{}

DMITIGR_PGFE_INLINE std::shared_ptr<Error> Sqlstate_exception::error() const noexcept
{
  return error_;
}

} // namespace dmitigr::pgfe
