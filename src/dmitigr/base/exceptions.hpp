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

#ifndef DMITIGR_BASE_EXCEPTIONS_HPP
#define DMITIGR_BASE_EXCEPTIONS_HPP

#include "error.hpp"

#include <exception>
#include <stdexcept>
#include <string>

namespace dmitigr {

/**
 * @returns The what-string of `ex`, or `unknown` if the underlying exception
 * is not derived from `std::exception`.
 */
inline std::string what(const std::exception_ptr& ex,
  const std::string& unknown = {})
{
  if (!ex)
    throw std::invalid_argument{"cannot get what string: exception_ptr is null"};

  try {
    rethrow_exception(ex);
  } catch (const std::exception& e) {
    return e.what();
  } catch (...) {
    return unknown;
  }
}

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public std::exception {
public:
  /**
   * @brief Constructs an instance associaterd with `errc`.
   *
   * @param errc The error code.
   * @param what The what-string.
   */
  explicit Exception(std::error_code errc, const std::string& what = {})
    : err_{errc, what}
  {}

  /**
   * @brief Constructs an instance associated with Errc::generic.
   *
   * @param what The what-string.
   */
  explicit Exception(const std::string& what)
    : Exception{Errc::generic, what}
  {}

  /// Constructs an instance from the object of type Err.
  explicit Exception(Err err)
    : err_{std::move(err)}
  {}

  /// @returns The what-string.
  const char* what() const noexcept override
  {
    return err_.what().c_str();
  }

  /// @returns The error code.
  std::error_code code() const noexcept
  {
    return err_.code();
  }

  /// @returns The underlying Err instance.
  const Err& err() const noexcept
  {
    return err_;
  }

private:
  Err err_;
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_EXCEPTIONS_HPP
