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

#ifndef DMITIGR_STR_SEQUENCE_HPP
#define DMITIGR_STR_SEQUENCE_HPP

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dmitigr::str {

// -----------------------------------------------------------------------------
// Sequence conversions
// -----------------------------------------------------------------------------

/// @returns The string with stringified elements of the sequence in range `[b, e)`.
template<class InputIterator, typename Function>
std::string to_string(InputIterator b, const InputIterator e,
  const std::string_view sep, const Function& to_str)
{
  std::string result;
  if (b != e) {
    while (true) {
      result.append(to_str(*b));
      ++b;
      if (b != e)
        result.append(sep);
      else
        break;
    }
  }
  return result;
}

/// @returns The string with stringified elements of the `Container`.
template<class Container, typename Function>
std::string to_string(const Container& cont, const std::string_view sep,
  const Function& to_str)
{
  return to_string(cbegin(cont), cend(cont), sep, to_str);
}

/// @returns The string with stringified elements of the `Container`.
template<class Container>
std::string to_string(const Container& cont, const std::string_view sep)
{
  return to_string(cont, sep, [](const std::string& e) -> const auto&
  {
    return e;
  });
}

/**
 * @brief Splits the `input` string into the parts by using the
 * specified `separators`.
 *
 * @param input An input string.
 * @param separators Separators.
 * @param to_type A converter from std::string_view to T.
 *
 * @returns The vector of splitted parts converted to T.
 */
template<class T, typename F>
std::vector<T> to_vector(const std::string_view input,
  const std::string_view separators, const F& to_type)
{
  using Size = std::string_view::size_type;
  std::vector<T> result;
  result.reserve(8);
  Size pos{std::string_view::npos};
  Size offset{};
  while (offset < input.size()) {
    pos = input.find_first_of(separators, offset);
    const auto part_size = std::min<Size>(pos, input.size()) - offset;
    result.push_back(to_type(input.substr(offset, part_size)));
    offset += part_size + 1;
  }
  if (pos != std::string_view::npos) // input ends with a separator
    result.push_back(to_type(std::string_view{}));
  return result;
}

/**
 * @brief Splits the `input` string into the parts separated by the
 * specified `separators`.
 *
 * @returns The vector of splitted parts.
 */
template<class S = std::string>
std::vector<S> to_vector(const std::string_view input,
  const std::string_view separators)
{
  return to_vector<S>(input, separators, [](const auto& v){return S{v};});
}

} // namespace dmitigr::str

#endif  // DMITIGR_STR_SEQUENCE_HPP
