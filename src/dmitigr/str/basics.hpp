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

#ifndef DMITIGR_STR_BASICS_HPP
#define DMITIGR_STR_BASICS_HPP

#include "../base/enum.hpp"

namespace dmitigr {
namespace str {

/// Denotes trimming mode bitmask.
enum class Trim {
  lhs = 0x1,
  rhs = 0x2,
  all = lhs | rhs
};

/// Denotes a byte format.
enum class Byte_format {
  raw = 1,
  hex
};

} // namespace str

template<> struct Is_bitmask_enum<str::Trim> : std::true_type {};

} // namespace dmitigr

#endif  // DMITIGR_STR_BASICS_HPP
