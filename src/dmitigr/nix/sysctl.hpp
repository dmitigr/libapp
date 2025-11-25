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

#if !defined(__FreeBSD__) && !defined(__APPLE__)
#error dmitigr/nix/sysctl.hpp is usable only on FreeBSD or macOS!
#endif

#ifndef DMITIGR_NIX_SYSCTL_HPP
#define DMITIGR_NIX_SYSCTL_HPP

#include "../base/str.hpp"

#include <string>
#include <system_error>

#include <sys/types.h>
#include <sys/sysctl.h>

namespace dmitigr::nix {

inline std::string sysctl(const std::string& name)
{
  static const auto throw_error = [](const int ev)
  {
    throw std::system_error{ev, std::system_category(),
      "cannot get system information"};
  };
  std::string result;
  std::size_t result_size{};
  if (!sysctlbyname(name.c_str(), nullptr, &result_size, nullptr, 0)) {
    result.resize(result_size);
    if (sysctlbyname(name.c_str(), result.data(), &result_size, nullptr, 0))
      throw_error(errno);
    str::trim(result);
  } else
    throw_error(errno);
  return result;
}

} // namespace dmitigr::nix

#endif  // DMITIGR_NIX_SYSCTL_HPP
