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

#ifndef DMITIGR_BASE_HASH_HPP
#define DMITIGR_BASE_HASH_HPP

#include <cstdint>
#include <string_view>

namespace dmitigr::hash {

/**
 * @returns CRC-16 calculated from the given data, or `0` if `(!data || !size)`.
 *
 * @tparam Poly Polynomial.
 * @param data The data for which CRC-16 has to be computed.
 * @param size The size of `data` in bytes.
 */
template<std::uint16_t Poly = 0x8005>
constexpr std::uint16_t crc16(const char* const data,
  const std::size_t size) noexcept
{
  static_assert(Poly > 1<<15, "Polynomial of CRC-16 must be 17 bits of length");
  if (!data || !size) return 0;

  std::uint16_t result{};
  bool is_high_bit_on{};

  // Process the words except the last one.
  std::uint8_t byte = data[0];
  for (std::size_t pos{}, bits_processed{}; pos < size;) {
    is_high_bit_on = result >> 15;
    result <<= 1;
    result |= (byte >> bits_processed) & 1;
    if (is_high_bit_on) result ^= Poly;

    ++bits_processed;
    if (bits_processed == 8) {
      ++pos;
      bits_processed = 0;
      byte = data[pos];
    }
  }
  // Process the last word.
  for (unsigned bits_processed{}; bits_processed < 16; ++bits_processed) {
    is_high_bit_on = result >> 15;
    result <<= 1;
    if (is_high_bit_on) result ^= Poly;
  }

  // Reverse the result.
  result = (result & 0x5555) << 1 | (result & 0xAAAA) >> 1;
  result = (result & 0x3333) << 2 | (result & 0xCCCC) >> 2;
  result = (result & 0x0F0F) << 4 | (result & 0xF0F0) >> 4;
  result = (result & 0x00FF) << 8 | (result & 0xFF00) >> 8;
  return result;
}
static_assert(!crc16(nullptr, 0));
static_assert(!crc16(nullptr, 7));
static_assert(!crc16("dmitigr", 0));
static_assert(crc16("dmitigr", 7) == 35600);

/// @overload
template<std::uint16_t Poly = 0x8005>
constexpr std::uint16_t crc16(const std::string_view data) noexcept
{
  return crc16<Poly>(data.data(), data.size());
}
static_assert(crc16("dmitigr") == 35600);

} // namespace dmitigr::hash

#endif  // DMITIGR_BASE_HASH_HPP
