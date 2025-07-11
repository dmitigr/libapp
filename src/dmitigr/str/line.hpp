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

#ifndef DMITIGR_STR_LINE_HPP
#define DMITIGR_STR_LINE_HPP

#include "exceptions.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// Lines
// -----------------------------------------------------------------------------

/**
 * @returns The line number (which starts at 0) by the given absolute position.
 *
 * @par Requires
 * `(pos < str.size())`.
 */
inline auto
line_number_by_position(const std::string& str, const std::string::size_type pos)
{
  if (!(pos < str.size()))
    throw Exception{"cannot get line number by invalid position"};

  using Diff = decltype(cbegin(str))::difference_type;
  return std::count(cbegin(str), cbegin(str) + static_cast<Diff>(pos), '\n');
}

/**
 * @returns The line and column numbers (both starts at 0) by the given absolute
 * position.
 *
 * @par Requires
 * `(pos < str.size())`.
 */
inline std::pair<std::size_t, std::size_t>
line_column_numbers_by_position(const std::string& str,
  const std::string::size_type pos)
{
  if (!(pos < str.size()))
    throw Exception{"cannot get line and column numbers by invalid position"};

  std::size_t line{};
  std::size_t column{};
  for (std::size_t i = 0; i < pos; ++i) {
    ++column;
    if (str[i] == '\n') {
      ++line;
      column = 0;
    }
  }
  return std::make_pair(line, column);
}

/**
 * @brief Extracts lines from `buffer` to `result`.
 *
 * @param result The destination vector.
 * @param buffer The source buffer.
 * @param is_remove_cr The directive to remove the carriage return character
 * from the end of each extracted line.
 *
 * @par Effects
 * `result` contains the extracted lines appended to the back.
 * `buffer` contains uncompleted line (if any) which is unextracted to `result`.
 *
 * @returns The number of extracted lines.
 */
inline std::string::size_type
get_lines(std::vector<std::string>& result, std::string& buffer,
  const bool is_remove_cr = true)
{
  std::string::size_type found_count{};
  std::string::size_type start_pos{};
  while (true) {
    const auto end_pos = buffer.find('\n', start_pos);
    if (end_pos != std::string::npos) {
      result.emplace_back(buffer.substr(start_pos, end_pos - start_pos));
      if (is_remove_cr && !result.back().empty() && result.back().back() == '\r')
        result.back().pop_back();
      start_pos = end_pos + 1;
      ++found_count;
    } else {
      if (start_pos) {
        if (start_pos < buffer.size())
          buffer = buffer.substr(start_pos);
        else
          buffer.clear();
      }
      break;
    }
  }
  return found_count;
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_LINE_HPP
