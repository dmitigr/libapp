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

#if !defined(__linux__) && !defined(__APPLE__)
#error dmitigr/nix/ifaddrs.hpp is usable only on Linux or macOS!
#endif

#ifndef DMITIGR_NIX_IFADDRS_HPP
#define DMITIGR_NIX_IFADDRS_HPP

#include "../str/transform.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// getifaddrs(3)
#include <sys/types.h>
#include <ifaddrs.h>

// packet(7)
#include <sys/socket.h>
#ifdef __linux__
#include <linux/if_packet.h>
#else
#include <net/if_dl.h>
#endif
#include <net/ethernet.h> /* the L2 protocols */

namespace dmitigr::nix {

/// A wrapper around ifaddrs.
class Ip_adapter_addresses final {
public:
  /// Constructs invalid instance.
  Ip_adapter_addresses()
    : data_{nullptr, &freeifaddrs}
  {}

  /// @warning May returns invalid instance.
  static Ip_adapter_addresses from_system()
  {
    Ip_adapter_addresses result;
    ifaddrs* data{};
    if (getifaddrs(&data))
      throw std::runtime_error{"cannot get network interface addresses"};
    result.data_.reset(data);
    return result;
  }

  /// @returns `true` is the instance is valid.
  bool is_valid() const noexcept
  {
    return static_cast<bool>(data_);
  }

  /// @returns `is_valid()`.
  explicit operator bool() const noexcept
  {
    return is_valid();
  }

  /// @returns The head of the linked list.
  const ifaddrs* head() const
  {
    if (!is_valid())
      throw std::logic_error{"cannot use invalid instance of type"
        " dmitigr::nix::Ip_adapter_addresses"};
    return reinterpret_cast<const ifaddrs*>(data_.get());
  }

  /// @overload
  ifaddrs* head()
  {
    return const_cast<ifaddrs*>(
      static_cast<const Ip_adapter_addresses*>(this)->head());
  }

private:
  std::unique_ptr<ifaddrs, void(*)(ifaddrs*)> data_;
};

/**
 * @returns A textual representation of a physical address of `iaa`.
 *
 * @par Requires
 * On Linux: `iaa.ifa_addr && iaa.ifa_addr->sa_family == AF_PACKET`.
 * On macOS: `iaa.ifa_addr && iaa.ifa_addr->sa_family == AF_LINK`.
 */
inline std::string physical_address_string(const ifaddrs& iaa,
  const std::string_view delimiter = "-")
{
#ifdef __linux__
  constexpr auto family = AF_PACKET;
#else
  constexpr auto family = AF_LINK;
#endif
  if (!iaa.ifa_addr || iaa.ifa_addr->sa_family != family)
    throw std::invalid_argument{"cannot get physical address: unexpected interface family"};

#ifdef __linux__
  using Link_level_sockaddr = sockaddr_ll;
#else
  using Link_level_sockaddr = sockaddr_dl;
#endif
  const auto* const sll = reinterpret_cast<const Link_level_sockaddr*>(iaa.ifa_addr);

#ifdef __linux__
  const std::string_view addr{reinterpret_cast<const char*>(sll->sll_addr), sll->sll_halen};
#else
  const std::string_view addr{LLADDR(sll), sll->sdl_alen};
#endif
  return dmitigr::str::sparsed_string(addr, dmitigr::str::Byte_format::hex, delimiter);
}

} // namespace dmitigr::nix

#endif  // DMITIGR_NIX_IFADDRS_HPP
