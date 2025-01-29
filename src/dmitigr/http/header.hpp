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

#ifndef DMITIGR_HTTP_HEADER_HPP
#define DMITIGR_HTTP_HEADER_HPP

#include "types_fwd.hpp"

#include <memory>
#include <string>

namespace dmitigr::http {

/**
 * @ingroup headers
 *
 * @brief A HTTP header.
 */
class Header {
public:
  /// The destructor.
  virtual ~Header() = default;

  /// @name Constructors
  /// @{

  /// @returns The copy of this instance.
  virtual std::unique_ptr<Header> to_header() const = 0;

  /// @}

  /// @returns The field name of the header in a HTTP message.
  virtual const std::string& field_name() const = 0;

  /// @name Conversions
  /// @{

  /**
   * @returns The result of conversion of this instance to the instance of
   * type `std::string`.
   */
  virtual std::string to_string() const = 0;

  /// @}

private:
  friend Cookie;
  friend Date;
  friend Set_cookie;

  Header() = default;
};

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_HEADER_HPP
