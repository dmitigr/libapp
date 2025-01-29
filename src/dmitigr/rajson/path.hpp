// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
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

#ifndef DMITIGR_RAJSON_PATH_HPP
#define DMITIGR_RAJSON_PATH_HPP

#include "exceptions.hpp"

#include <deque>
#include <iterator>
#include <string>
#include <utility>

namespace dmitigr::rajson {

/**
 * @brief Denotes a path to JSON member.
 */
class Path final {
public:
  /// Constructs an empty path.
  Path() = default;

  /**
   * @brief The constructor.
   *
   * @par Requires
   * `!path.empty()`.
   */
  explicit Path(const std::string_view path, std::string separator = "/")
    : separator_{std::move(separator)}
  {
    if (path.empty())
      throw Exception{"cannot construct JSON path from empty string"};
    else if (separator_.empty())
      throw Exception{"cannot construct JSON path with empty separator"};

    const auto pathsz = path.size();
    auto seppos = path.find(separator_), compos = seppos - seppos;
    while (seppos != std::string_view::npos) {
      const auto comsz = seppos - compos;
      components_.push_back(std::string{path.substr(compos, comsz)});
      compos += comsz + 1;
      if (compos >= pathsz)
        throw Exception{"cannot construct JSON path with empty last component"};
      seppos = path.find(separator_, compos);
    }
    components_.push_back(std::string{path.substr(compos)});
  }

  /// @returns `true` if this path is empty.
  bool is_empty() const noexcept
  {
    return components_.empty();
  }

  /// Appends the `path` to this instance.
  Path& operator/(Path path)
  {
    components_.insert(cend(components_),
      std::make_move_iterator(cbegin(path.components_)),
      std::make_move_iterator(cend(path.components_)));
    return *this;
  }

  /**
   * @overload
   *
   * @par Requires
   * `!path.empty()`.
   */
  Path& operator/(const std::string_view path)
  {
    if (path.empty())
      throw Exception{"cannot append empty component to JSON path"};
    return operator/(Path{path});
  }

  /// Removes the first component from this instance.
  void remove_front_component()
  {
    if (components_.empty())
      throw Exception{"cannot remove first component from empty JSON path"};
    components_.pop_front();
  }

  /// Removes the last component from this instance.
  void remove_back_component()
  {
    if (components_.empty())
      throw Exception{"cannot remove last component from empty JSON path"};
    components_.pop_back();
  }

  /// @returns The separator.
  const std::string& separator() const noexcept
  {
    return separator_;
  }

  /// @returns The deque that represents the path.
  const std::deque<std::string>& components() const noexcept
  {
    return components_;
  }

  /// @returns The string representation of this instance.
  std::string to_string() const
  {
    std::string result;
    if (!components_.empty()) {
      for (const auto& comp : components_)
        result.append(comp).append(separator_);
      result.resize(result.size() - separator_.size());
    }
    return result;
  }

private:
  std::string separator_;
  std::deque<std::string> components_;
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_PATH_HPP
