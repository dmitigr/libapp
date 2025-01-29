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

#include <stdexcept>

namespace dmitigr::os {

/// OS family.
enum class Family {
  lin = 1,
  macos,
  windows
};

/// @returns An OS family literal.
constexpr const char* to_literal(const Family family)
{
  using F = Family;
  switch (family) {
  case F::lin:
    return "linux";
  case F::macos:
    return "macos";
  case F::windows:
    return "windows";
  }
  throw std::invalid_argument{"unsupported OS family"};
}

/// @returns OS family from `value`.
constexpr Family to_family(const std::string_view value)
{
  using F = Family;
  if (value == "linux")
    return F::lin;
  else if (value == "macos")
    return F::macos;
  else if (value == "windows")
    return F::windows;
  throw std::invalid_argument{"unsupported OS family"};
}

/// @returns OS family.
constexpr Family family() noexcept
{
  using F = Family;
#if __linux__
  return F::lin;
#elif __APPLE__
  return F::macos;
#elif _WIN32
  return F::windows;
#else
  static_assert(false, "unsupported OS family");
#endif
}

} // namespace dmitigr::os
