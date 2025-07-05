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

#ifndef DMITIGR_MAC_IO_HPP
#define DMITIGR_MAC_IO_HPP

#ifndef __APPLE__
#error dmitigr/mac/io.hpp is usable only on macOS!
#endif

#include "cf.hpp"

#include <stdexcept>
#include <string>

#include <IOKit/IOKitLib.h>

namespace dmitigr::mac::io {

inline std::string platform_uuid()
{
  const io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault,
    IOServiceMatching("IOPlatformExpertDevice"));
  const auto uuid = cf::String::created(static_cast<CFStringRef>(
    IORegistryEntryCreateCFProperty(service, CFSTR("IOPlatformUUID"),
      kCFAllocatorDefault, 0)));
  const char* const uuid_c_str = CFStringGetCStringPtr(uuid.native(),
    kCFStringEncodingASCII);
  if (!uuid_c_str)
    throw std::runtime_error{"cannot get IOPlatformUUID from IOPlatformExpertDevice"};
  return uuid_c_str;
}

} // namespace dmitigr::mac::io

#endif  // DMITIGR_MAC_IO_HPP
