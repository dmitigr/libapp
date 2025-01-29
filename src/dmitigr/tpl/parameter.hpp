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

#ifndef DMITIGR_TPL_PARAMETER_HPP
#define DMITIGR_TPL_PARAMETER_HPP

#include "types_fwd.hpp"

#include <optional>
#include <string>
#include <utility>

namespace dmitigr::tpl {

/// A template parameter.
class Parameter final {
public:
  /// The constructor.
  explicit Parameter(std::string name,
    std::optional<std::string> value = {})
    : name_{std::move(name)}
    , value_{std::move(value)}
  {}

  /// @returns The parameter name.
  const std::string& name() const noexcept
  {
    return name_;
  }

  /// @returns The parameter value.
  const std::optional<std::string>& value() const noexcept
  {
    return value_;
  }

  /// Sets the value of parameter.
  void set_value(std::optional<std::string> value)
  {
    value_ = std::move(value);
  }

private:
  friend Generic;
  std::string name_;
  std::optional<std::string> value_;
};

/// @returns `true` if `lhs` is less than `rhs`.
inline bool operator<(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() < rhs.name() && lhs.value() < rhs.value();
}

/// @returns `true` if `lhs` is less or equals to `rhs`.
inline bool operator<=(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() <= rhs.name() && lhs.value() <= rhs.value();
}

/// @returns `true` if `lhs` is equals to `rhs`.
inline bool operator==(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() == rhs.name() && lhs.value() == rhs.value();
}

/// @returns `true` if `lhs` is not equals to `rhs`.
inline bool operator!=(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() != rhs.name() && lhs.value() != rhs.value();
}

/// @returns `true` if `lhs` is greater or equals to `rhs`.
inline bool operator>=(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() >= rhs.name() && lhs.value() >= rhs.value();
}

/// @returns `true` if `lhs` is greater than `rhs`.
inline bool operator>(const Parameter& lhs, const Parameter& rhs) noexcept
{
  return lhs.name() > rhs.name() && lhs.value() > rhs.value();
}

} // namespace dmitigr::tpl

#endif  // DMITIGR_TPL_PARAMETER_HPP
