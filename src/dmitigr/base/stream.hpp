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

#ifndef DMITIGR_BASE_STREAM_HPP
#define DMITIGR_BASE_STREAM_HPP

#include "exceptions.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <istream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

namespace dmitigr {

/// @returns Size of `in` to read.
inline auto seekg_size(std::istream& in, const std::istream::off_type offset = 0)
{
  using std::ios_base;
  constexpr const char* const errmsg{"cannot get size of input stream"};
  if (!in.seekg(offset, ios_base::end))
    throw Exception{errmsg};
  const auto result = in.tellg();
  if (!in)
    throw Exception{errmsg};
  if (!in.seekg(offset, ios_base::beg))
    throw Exception{errmsg};
  return result;
}

/**
 * @brief Reads lines from the `input` stream.
 *
 * @param callback The function of signature `bool callback(const std::string&)`
 * that returns `true` to continue the reading.
 * @param input The input stream to read the data from.
 * @param delimiter The delimiter character.
 */
template<typename F, typename Pred>
std::istream& read_lines_if(F&& callback, std::istream& input,
  const char delimiter = '\n')
{
  std::string line;
  while (getline(input, line, delimiter)) {
    if (!callback(line))
      break;
    line.clear();
  }
  return input;
}

/**
 * @brief Reads the `input` stream by chunks of size `BufSize`.
 *
 * @tparam ChunkSize The chunk size.
 * @param callback The function of signature
 * `bool callback(const char*, const std::size_t)` that returns `true`
 * to continue the reading.
 * @param input The input stream to read the data from.
 * @param delimiter The delimiter character.
 *
 * @par Requires
 * `ChunkSize && !(ChunkSize % 8)`.
 */
template<std::size_t ChunkSize, typename F>
std::istream& read(F&& callback, std::istream& input)
{
  static_assert(ChunkSize);
  std::array<char, ChunkSize> buffer;
  static_assert(!(buffer.size() % 8));
  const auto append = [&]
  {
    return callback(buffer.data(), static_cast<std::size_t>(input.gcount()));
  };
  while (input.read(buffer.data(), buffer.size())) {
    if (!append())
      return input;
  }
  if (input.gcount())
    append();
  return input;
}

/// @overload
template<typename F>
std::istream& read(F&& callback, std::istream& input)
{
  return read<4096>(std::forward<F>(callback), input);
}

/// Reads the whole `input` to string.
inline std::string read_to_string(std::istream& input)
{
  std::string result;
  read([&result](const char* const data, const std::size_t size)
  {
    result.append(data, size);
    return true;
  }, input);
  return result;
}

/// @overload
inline std::string read_to_string(const std::filesystem::path& path)
{
  if (std::ifstream input{path, std::ios_base::binary})
    return read_to_string(input);
  throw Exception{"cannot open file "+path.string()+" for reading"};
}

} // namespace dmitigr

#endif  // DMITIGR_BASE_STREAM_HPP
