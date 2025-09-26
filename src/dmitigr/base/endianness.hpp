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

#ifndef DMITIGR_BASE_ENDIANNESS_HPP
#define DMITIGR_BASE_ENDIANNESS_HPP

#ifdef _WIN32
#pragma comment(lib, "Ws2_32")

#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <cstdint>

namespace dmitigr {

/// An endianness.
enum class Endianness {
  /// Big-endian.
  big,
  /// Little-endian.
  little
};

/// @returns Endianness of the system.
inline Endianness endianness() noexcept
{
  static_assert(sizeof(unsigned char) < sizeof(unsigned long),
    "unknown endianness");
  static const unsigned long number{0x01};
  static const auto result =
    (reinterpret_cast<const unsigned char*>(&number)[0] == 1)
    ? Endianness::little : Endianness::big;
  return result;
}

/// @returns `value` in network byte order converted from host byte order.
inline auto host_to_net(const std::uint16_t value) noexcept
{
  return htons(value);
}

/// @overload
inline auto host_to_net(const std::uint32_t value) noexcept
{
  return htonl(value);
}

/// @returns `value` in host byte order converted from network byte order.
inline auto net_to_host(const std::uint16_t value) noexcept
{
  return ntohs(value);
}

/// @overload
inline auto net_to_host(const std::uint32_t value) noexcept
{
  return ntohl(value);
}

} // namespace dmitigr

#endif  // DMITIGR_BASE_ENDIANNESS_HPP
