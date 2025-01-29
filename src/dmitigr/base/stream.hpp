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

#ifndef DMITIGR_BASE_STREAM_HPP
#define DMITIGR_BASE_STREAM_HPP

#include <istream>
#include <stdexcept>

namespace dmitigr {

/// @returns Size of `in` to read.
inline auto seekg_size(std::istream& in, const std::istream::off_type offset = 0)
{
  using std::ios_base;
  constexpr const char* const errmsg{"cannot get size of input stream"};
  if (!in.seekg(offset, ios_base::end))
    throw std::runtime_error{errmsg};
  const auto result = in.tellg();
  if (!in)
    throw std::runtime_error{errmsg};
  if (!in.seekg(offset, ios_base::beg))
    throw std::runtime_error{errmsg};
  return result;
}

} // namespace dmitigr

#endif  // DMITIGR_BASE_STREAM_HPP
