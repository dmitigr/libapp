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

#include "basics.hpp"
#include "exceptions.hpp"
#include "misc.hpp"
#include "pq.hpp"

#include <cctype>

namespace dmitigr::pgfe {

DMITIGR_PGFE_INLINE void set_lazy_initialization(const External_library library)
{
  const auto libssl = static_cast<bool>(library & External_library::libssl);
  const auto libcrypto = static_cast<bool>(library & External_library::libcrypto);
  PQinitOpenSSL(libssl, libcrypto);
}

DMITIGR_PGFE_INLINE int array_dimension(const std::string_view literal,
  const char delimiter)
{
  if (!literal.data())
    return 0;

  int dimension{};
  for (const auto c : literal) {
    if (c == '{')
      ++dimension;
    else if (std::isspace(static_cast<unsigned char>(c)))
      ; // Skip space.
    else if (!dimension || c == delimiter)
      throw Generic_exception{Errc::malformed_literal};
    else
      break;
  }
  return dimension;
}

} // namespace dmitigr::pgfe
