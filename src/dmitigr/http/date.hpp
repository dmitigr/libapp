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

#ifndef DMITIGR_HTTP_DATE_HPP
#define DMITIGR_HTTP_DATE_HPP

#include "../dt/timestamp.hpp"
#include "header.hpp"

#include <string_view>

namespace dmitigr::http {

/**
 * @ingroup headers
 *
 * @brief A HTTP Date header.
 */
class Date final : public Header {
public:
  /**
   * @brief Constructs the object by parsing the `input`.
   *
   * @details Examples of valid input:
   *   -# Sat, 06 Apr 2019 17:00:00 GMT
   *
   * @param input - the http-date.
   *
   * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Date
   */
  explicit Date(const std::string_view input)
    : Date{dt::Timestamp::from_rfc7231(input)}
  {}

  /**
   * @overload
   *
   * @par Requires
   * `(ts != nullptr)`.
   */
  explicit Date(dt::Timestamp ts)
    : ts_{std::move(ts)}
  {}

  /// @see Header::to_header().
  std::unique_ptr<Header> to_header() const override
  {
    return std::make_unique<Date>(*this);
  }

  /// @see Header::field_name().
  const std::string& field_name() const override
  {
    static const std::string result{"Date"};
    return result;
  }

  /// @see Header::to_string().
  std::string to_string() const override
  {
    return ts_.to_rfc7231();
  }

  /// @returns The timestamp.
  const dt::Timestamp& timestamp() const noexcept
  {
    return ts_;
  }

  /// @overload
  dt::Timestamp& timestamp() noexcept
  {
    return const_cast<dt::Timestamp&>(
      static_cast<const Date*>(this)->timestamp());
  }

  /**
   * @brief Sets the timestamp.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @par Requires
   * `(ts != nullptr)`.
   */
  void set_timestamp(dt::Timestamp ts)
  {
    ts_ = std::move(ts);
  }

private:
  dt::Timestamp ts_;
};

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_DATE_HPP
