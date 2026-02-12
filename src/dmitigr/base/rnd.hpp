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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace dmitigr::rnd {

// -----------------------------------------------------------------------------
// Number generators
// -----------------------------------------------------------------------------

/// Represents a concept of uniformly distributed integer.
template<typename T>
concept Uniformly_distributed_integer =
  std::is_same_v<short, T> || std::is_same_v<int, T> ||
  std::is_same_v<long, T> || std::is_same_v<long long, T> ||
  std::is_same_v<unsigned short, T> || std::is_same_v<unsigned int, T> ||
  std::is_same_v<unsigned long, T> || std::is_same_v<unsigned long long, T>;


/**
 * @returns The random number in range `[minimum, maximum]`.
 *
 * @par Requires
 * `(minimum <= maximum)`.
 */
template<typename T>
requires Uniformly_distributed_integer<T>
T ud_integer(const T minimum, const T maximum)
{
  if (!(minimum <= maximum))
    throw std::invalid_argument{"invalid interval for random integer generation"};
  static std::mt19937 gen{std::random_device{}()};
  return std::uniform_int_distribution<T>{minimum, maximum}(gen);
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
      result[i] = palette[ud_integer<std::string::size_type>(0, pallete_size - 1)];
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
    throw std::invalid_argument{"invalid character range for random string generation"};

  std::string result;
  if (beg < end) {
    result.resize(size);
    const auto len = end - beg;
    for (std::string::size_type i{}; i < size; ++i)
      result[i] = static_cast<char>((ud_integer<unsigned int>(0, end) % len) + beg);
  }
  return result;
}

// -----------------------------------------------------------------------------
// Uuid
// -----------------------------------------------------------------------------

/// An UUID.
class Uuid final {
public:
  /// An alias of raw representation type.
  using Raw = unsigned char[16];

  /// Constructs Nil UUID.
  Uuid() noexcept = default;

  /// Constructs UUID by using a raw representation.
  Uuid(const Raw& raw) noexcept
  {
    std::memcpy(data_.raw_, raw, sizeof(raw));
  }

  /**
   * @returns The random UUID (version 4).
   *
   * @par Thread-safety
   * Thread-safe.
   */
  static Uuid make_v4() noexcept
  {
    Uuid result;

    // Filling the data with random bytes.
    {
      for (int i{}; i < 2; ++i) {
        const auto num = generator_();
        static_assert(sizeof(num) == 8);
        static_assert(sizeof(result.data_.raw_) == 2*sizeof(num));
        std::memcpy(result.data_.raw_ + i*sizeof(num), &num, sizeof(num));
      }
    }

    /*
     * Setting magic numbers for the "version 4" (pseudorandom) UUID.
     * See http://tools.ietf.org/html/rfc4122#section-4.4
     */
    result.data_.rep_.time_hi_and_version_ =
      (result.data_.rep_.time_hi_and_version_ & 0x0fff) | 0x4000;
    result.data_.rep_.clock_seq_hi_and_reserved_ =
      (result.data_.rep_.clock_seq_hi_and_reserved_ & 0x3f) | 0x80;

    return result;
  }

  /// @returns The string representation of the UUID.
  std::string to_string() const
  {
    constexpr std::size_t buf_size{36};
    char buf[buf_size + 1];
    const int count{std::snprintf(buf, sizeof(buf),
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
      data_.rep_.node_[5])};
    DMITIGR_ASSERT(count == buf_size);
    return std::string{buf, buf_size};
  }

  /// @returns The raw representation of the UUID.
  const Raw& raw() const noexcept
  {
    return data_.raw_;
  }

private:
  inline static thread_local std::mt19937_64 generator_{std::random_device{}()};
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
};

} // namespace dmitigr::rnd

#endif  // DMITIGR_BASE_RND_HPP
