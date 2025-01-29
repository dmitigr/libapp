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

#ifndef DMITIGR_BASE_RND_HPP
#define DMITIGR_BASE_RND_HPP

#include "assert.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>

namespace dmitigr::rnd {

// -----------------------------------------------------------------------------
// Number generators
// -----------------------------------------------------------------------------

/// Seeds the pseudo-random number generator.
inline void seed_by_now() noexcept
{
  const auto seed = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  std::srand(static_cast<unsigned>(seed));
}

/**
 * @returns The random number in range `[minimum, maximum]`.
 *
 * @par Requires
 * `(minimum <= maximum)`.
 */
template<typename T>
T week_integer(const T minimum, const T maximum)
{
  if ((minimum < 0) || !(minimum <= maximum))
    throw Exception{"invalid interval for random number generation"};

  static const auto rand_num = [](const auto up) noexcept
  {
    const auto num = static_cast<double>(std::rand() + 1);
    return static_cast<T>(up * (num / RAND_MAX));
  };

  const auto range_length = maximum - minimum + 1;
  return (rand_num(maximum + !minimum) % range_length) + minimum;
}

// -----------------------------------------------------------------------------
// String generators
// -----------------------------------------------------------------------------

/**
 * @returns The random string.
 *
 * @param size The result size.
 * @param palette The palette of characters the result will consist of.
 */
inline std::string str(const std::string& palette,
  const std::string::size_type size)
{
  std::string result;
  result.resize(size);
  if (const auto pallete_size = palette.size()) {
    for (std::string::size_type i{}; i < size; ++i)
      result[i] = palette[week_integer<std::string::size_type>(0, pallete_size - 1)];
  }
  return result;
}

/**
 * @returns The random string.
 *
 * @param beg The start of source range.
 * @param end The past of end of source range.
 * @param size The result size.
 *
 * @par Requires
 * `(beg <= end)`.
 */
inline std::string str(const char beg, const char end,
  const std::string::size_type size)
{
  if (!(beg <= end))
    throw Exception{"invalid character range for random string generation"};

  std::string result;
  if (beg < end) {
    result.resize(size);
    const auto len = end - beg;
    for (std::string::size_type i{}; i < size; ++i)
      result[i] = static_cast<char>((week_integer<char>(0, end) % len) + beg);
  }
  return result;
}

// -----------------------------------------------------------------------------
// Uuid
// -----------------------------------------------------------------------------

/// An UUID.
class Uuid final {
public:
  /// Constructs Nil UUID.
  Uuid() = default;

  /// An alias of raw representation type.
  using Raw = std::array<unsigned char, 16>;

  /// Constructs UUID by using a raw representation.
  Uuid(const Raw& raw)
  {
    data_.raw_ = raw;
  }

  /**
   * @returns The random UUID (version 4).
   *
   * @remarks Please be sure to seed the pseudo-random number generator with
   * `std::srand()` (or use convenient `dmitigr::rnd::seed_by_now()`) before
   * calling this maker function.
   */
  static Uuid make_v4()
  {
    Uuid result;

    // Filling the data with random bytes.
    {
      constexpr auto minimum = static_cast<unsigned char>(1);
      constexpr auto maximum = std::numeric_limits<unsigned char>::max();
      for (std::size_t i{}; i < sizeof(result.data_.raw_); ++i)
        result.data_.raw_[i] = week_integer(minimum, maximum);
    }

    /*
     * Setting magic numbers for the "version 4" (pseudorandom) UUID.
     * See http://tools.ietf.org/html/rfc4122#section-4.4
     */
    result.data_.rep_.time_hi_and_version_ =
      (result.data_.rep_.time_hi_and_version_ & 0x0fff) | 0x4000;
    result.data_.rep_.clock_seq_hi_and_reserved_ =
      (result.data_.rep_.clock_seq_hi_and_reserved_ & 0x3f) | 0x80;

    DMITIGR_ASSERT(result.is_invariant_ok());
    return result;
  }

  /// @returns The string representation of the UUID.
  std::string to_string() const
  {
    constexpr std::size_t buf_size{36};
    char buf[buf_size + 1];
    const int count = std::snprintf(buf, sizeof(buf),
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      data_.rep_.time_low_,
      data_.rep_.time_mid_,
      data_.rep_.time_hi_and_version_,
      data_.rep_.clock_seq_hi_and_reserved_,
      data_.rep_.clock_seq_low_,
      data_.rep_.node_[0],
      data_.rep_.node_[1],
      data_.rep_.node_[2],
      data_.rep_.node_[3],
      data_.rep_.node_[4],
      data_.rep_.node_[5]);
    DMITIGR_ASSERT(count == buf_size);
    return std::string{buf, buf_size};
  }

  /// @returns The raw representation of the UUID.
  const Raw& raw() const noexcept
  {
    return data_.raw_;
  }

private:
  union {
    struct {
      std::uint32_t time_low_;
      std::uint16_t time_mid_;
      std::uint16_t time_hi_and_version_;
      std::uint8_t clock_seq_hi_and_reserved_;
      std::uint8_t clock_seq_low_;
      std::uint8_t node_[6];
    } rep_;
    Raw raw_{};
  } data_;

  bool is_invariant_ok() const noexcept
  {
    return std::any_of(std::cbegin(data_.raw_), std::cend(data_.raw_),
      [](const auto b) { return b != 0; });
  }
};

} // namespace dmitigr::rnd

#endif  // DMITIGR_BASE_RND_HPP
