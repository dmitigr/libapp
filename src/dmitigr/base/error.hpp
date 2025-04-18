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

#ifndef DMITIGR_BASE_ERROR_HPP
#define DMITIGR_BASE_ERROR_HPP

#include <cstring> // std::strlen
#include <string>
#include <system_error>
#include <utility>

namespace dmitigr {

/**
 * @ingroup errors
 *
 * @brief Generic error codes.
 */
enum class Errc {
  /// Generic error.
  generic = 1,
};

/**
 * @ingroup errors
 *
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::generic: return "generic";
  }
  return nullptr;
}

/**
 * @ingroup errors
 *
 * @returns The literal returned by `to_literal(errc)`, or literal
 * "unknown error" if `to_literal(errc)` returned `nullptr`.
 */
template<typename E>
constexpr const char* to_literal_anyway(const E errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace dmitigr

// -----------------------------------------------------------------------------
// Standard library integration
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_code_enum<dmitigr::Errc> final : true_type {};

} // namespace std

namespace dmitigr {

/**
 * @ingroup errors
 *
 * @brief A generic category of errors.
 *
 * @see Exception.
 */
class Generic_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_generic_error`.
  const char* name() const noexcept override
  {
    return name_;
  }

  /**
   * @returns The string that describes the error code denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    const char* const desc{to_literal_anyway(static_cast<Errc>(ev))};
    constexpr const char sep[]{": "};
    std::string result;
    result.reserve(std::size(name_) - 1 + std::size(sep) - 1 + std::strlen(desc));
    return result.append(name_).append(sep).append(desc);
  }

private:
  static constexpr const char name_[]{"dmitigr_generic_error"};
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Generic_error_category.
 */
inline const Generic_error_category& generic_error_category() noexcept
{
  static const Generic_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), generic_error_category())`.
 */
inline std::error_code make_error_code(const Errc errc) noexcept
{
  return {static_cast<int>(errc), generic_error_category()};
}

// -----------------------------------------------------------------------------
// Err
// -----------------------------------------------------------------------------

/// An error.
class Err final {
public:
  /// Constructs not an error.
  Err() noexcept = default;

  /// The constructor.
  explicit Err(std::error_code code, std::string what = {}) noexcept
    : code_{std::move(code)}
    , what_{std::move(what)}
  {}

  /// @returns `true` if the instance represents an error.
  explicit operator bool() const noexcept
  {
    return static_cast<bool>(code_);
  }

  /// @returns The error code.
  std::error_code code() const noexcept
  {
    return code_;
  }

  /// @returns The what-string.
  const std::string& what() const noexcept
  {
    return what_;
  }

  /// @returns The error message combined from `code().message()` and `what()`.
  const std::string& message() const noexcept
  {
    try {
      if (message_.empty()) {
        message_ = code_.message();
        if (!what_.empty())
          message_.append(": ").append(what_);
      }
      return message_;
    } catch (...) {
      message_.clear();
    }
    return what_;
  }

private:
  std::error_code code_;
  std::string what_;
  mutable std::string message_;
};

/// @returns `true` if `lhs` is equals to `rhs`.
inline bool operator==(const Err& lhs, const std::error_code& rhs) noexcept
{
  return lhs.code() == rhs;
}

/// @overload
inline bool operator==(const std::error_code& lhs, const Err& rhs) noexcept
{
  return lhs == rhs.code();
}

/// @overload
inline bool operator==(const Err& lhs, const Err& rhs) noexcept
{
  return lhs.code() == rhs.code();
}

/// @returns `true` if `lhs` is not equals to `rhs`.
inline bool operator!=(const Err& lhs, const std::error_code& rhs) noexcept
{
  return !(lhs.code() == rhs);
}

/// @overload
inline bool operator!=(const std::error_code& lhs, const Err& rhs) noexcept
{
  return !(lhs == rhs.code());
}

/// @overload
inline bool operator!=(const Err& lhs, const Err& rhs) noexcept
{
  return !(lhs == rhs);
}

} // namespace dmitigr

#endif  // DMITIGR_BASE_ERROR_HPP
