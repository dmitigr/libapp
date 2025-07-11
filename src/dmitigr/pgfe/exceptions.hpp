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

#ifndef DMITIGR_PGFE_EXCEPTIONS_HPP
#define DMITIGR_PGFE_EXCEPTIONS_HPP

#include "../base/exceptions.hpp"
#include "dll.hpp"
#include "errc.hpp"
#include "types_fwd.hpp"

#include <memory>
#include <string>
#include <utility>

namespace dmitigr::pgfe {

/**
 * @ingroup errors
 *
 * @brief The base exception class.
 */
class Exception : public dmitigr::Exception {
private:
  friend Generic_exception;
  friend Sqlstate_exception;

  template<typename ... Types>
  Exception(Types&& ... args)
    : dmitigr::Exception{std::forward<Types>(args)...}
  {}
};

// =============================================================================

/**
 * @ingroup errors
 *
 * @brief A generic exception.
 */
class Generic_exception final : public Exception {
public:
  /// The constructor.
  explicit DMITIGR_PGFE_API Generic_exception(const Errc errc,
    std::string what = {});

  /// @overload
  explicit DMITIGR_PGFE_API Generic_exception(const std::string& what);
};

// =============================================================================

/**
 * @ingroup errors
 *
 * @brief An exception associated with SQLSTATE error condition.
 */
class Sqlstate_exception final : public Exception {
public:
  /**
   * @brief The constructor.
   *
   * @par Requires
   * `error`.
   */
  explicit DMITIGR_PGFE_API Sqlstate_exception(std::shared_ptr<Error> error);

  /// @returns The error response (aka error report).
  DMITIGR_PGFE_API std::shared_ptr<Error> error() const noexcept;

private:
  std::shared_ptr<Error> error_;
};

} // namespace dmitigr::pgfe

#ifndef DMITIGR_PGFE_NOT_HEADER_ONLY
#include "exceptions.cpp"
#endif

#endif  // DMITIGR_PGFE_EXCEPTIONS_HPP
