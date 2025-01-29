// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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

// IP Helper

#pragma once
#pragma comment(lib, "iphlpapi")

#include "../base/assert.hpp"
#include "../str/transform.hpp"
#include "exceptions.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <winsock2.h>
#include <iphlpapi.h>

namespace dmitigr::winbase::iphelper {

/// A wrapper around IP_ADAPTER_ADDRESSES.
class Ip_adapter_addresses final {
public:
  /// Constructs invalid instance.
  Ip_adapter_addresses() = default;

  /// @warning May returns invalid instance.
  static Ip_adapter_addresses from_system(const ULONG family, const ULONG flags)
  {
    constexpr PVOID reserved{};
    ULONG size{};
    if (const auto e = GetAdaptersAddresses(
        family, flags, reserved, nullptr, &size); e != ERROR_BUFFER_OVERFLOW)
      throw Sys_exception{e, "cannot determine size to retrieve network adapters"
        " addresses"};

    Ip_adapter_addresses result;
    if (size) {
      result.data_.resize(size);
      if (const auto e = GetAdaptersAddresses(
          family, flags, reserved, result.head(), &size))
        throw Sys_exception{e, "cannot retrieve network adapters addresses"};
    }
    return result;
  }

  /// @returns `true` is the instance is valid.
  bool is_valid() const noexcept
  {
    return !data_.empty();
  }

  /// @returns `is_valid()`.
  explicit operator bool() const noexcept
  {
    return is_valid();
  }

  /// @returns The head of the linked list.
  const IP_ADAPTER_ADDRESSES* head() const
  {
    if (!is_valid())
      throw std::logic_error{"cannot use invalid instance of type"
        " dmitigr::winbase::iphelp::Ip_adapter_addresses"};
    return reinterpret_cast<const IP_ADAPTER_ADDRESSES*>(data_.data());
  }

  /// @overload
  IP_ADAPTER_ADDRESSES* head()
  {
    return const_cast<IP_ADAPTER_ADDRESSES*>(
      static_cast<const Ip_adapter_addresses*>(this)->head());
  }

private:
  std::vector<BYTE> data_;
};

/**
 * @returns A textual representation of `iaa.PhysicalAddress`, or empty
 * string if `!iaa.PhysicalAddressLength`.
 */
inline std::string physical_address_string(const IP_ADAPTER_ADDRESSES& iaa,
  const std::string_view delimiter = "-")
{
  return dmitigr::str::sparsed_string(std::string_view{
    reinterpret_cast<const char*>(iaa.PhysicalAddress),
    iaa.PhysicalAddressLength}, dmitigr::str::Byte_format::hex, delimiter);
}

} // namespace dmitigr::winbase::iphelper
