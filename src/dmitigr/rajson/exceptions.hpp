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

#ifndef DMITIGR_RAJSON_EXCEPTIONS_HPP
#define DMITIGR_RAJSON_EXCEPTIONS_HPP

#include "../base/exceptions.hpp"
#include "errctg.hpp"

#include <stdexcept> // std::runtime_error

namespace dmitigr::rajson {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public dmitigr::Exception {
  using dmitigr::Exception::Exception;
};

// -----------------------------------------------------------------------------
// Parse_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The exception denotes parse error.
 */
class Parse_exception final : public Exception {
public:
  /// The constructor.
  explicit Parse_exception(const rapidjson::ParseResult pr)
    : Exception{make_error_code(pr.Code()),
      rapidjson::GetParseError_En(pr.Code())}
    , pr_{pr}
  {}

  /// @returns A parse result.
  const rapidjson::ParseResult& parse_result() const noexcept
  {
    return pr_;
  }

private:
  rapidjson::ParseResult pr_;
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_EXCEPTIONS_HPP
