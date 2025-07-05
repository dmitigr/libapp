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

#ifndef DMITIGR_RAJSON_DOCUMENT_HPP
#define DMITIGR_RAJSON_DOCUMENT_HPP

#include "../rajson/conversions.hpp"
#include "../rajson/value_view.hpp"
#include "../str/stream.hpp"
#include "exceptions.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace dmitigr::rajson {

class Document final {
public:
  /// The default-constructible.
  Document() = default;

  /// Parses from `json`.
  explicit Document(const std::string_view json) try
    : doc_{rajson::document_from_text(json)}
  {
    if (!doc_.IsNull() && !doc_.IsObject())
      throw Exception{"invalid JSON input"};
  } catch (const rajson::Parse_exception& e) {
    throw Exception{std::string{"JSON parse error near position "}
      .append(std::to_string(e.parse_result().Offset()))};
  }

  /**
   * @brief Parses from `file`.
   *
   * @par Requires
   * `!file.empty() && std::filesystem::exists(file)`.
   *
   * @returns The new instance.
   */
  explicit Document(const std::filesystem::path& file)
  {
    if (file.empty())
      throw Exception{"empty path to JSON file specified"};

    if (!std::filesystem::exists(file))
      throw Exception{"JSON file "+file.string()+" not found"};

    Document tmp{std::string_view{str::read_to_string(file)}};
    swap(tmp);
  }

  /// Copy-constructible.
  Document(const Document& rhs) noexcept
  {
    doc_.CopyFrom(rhs.doc_, doc_.GetAllocator(), true);
  }

  /// Copy-assignable.
  Document& operator=(const Document& rhs) noexcept
  {
    Document tmp{rhs};
    swap(tmp);
    return *this;
  }

  /// Move-constructible.
  Document(Document&& rhs) noexcept
    : doc_{std::move(rhs.doc_)}
  {}

  /// Move-assignable.
  Document& operator=(Document&& rhs) noexcept
  {
    Document tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  /// Swaps this instance with the `other`.
  void swap(Document& other) noexcept
  {
    using std::swap;
    swap(doc_, other.doc_);
  }

  /// @returns Parsed JSON as rajson::Value_view.
  const auto json() const noexcept
  {
    return rajson::Value_view{doc_};
  }

  /// @overload
  auto json() noexcept
  {
    return rajson::Value_view{doc_};
  }

  /// Assigns the value of `names` found in `json()` to `dst`.
  template<typename Dest>
  void get(Dest& dst, const std::string_view name) const
  {
    try {
      json().get(dst, name);
    } catch (const std::exception& e) {
      throw Exception{std::string{"cannot get value of JSON node "}
        .append(name).append(": ").append(e.what())};
    }
  }

  /// Assigns the value of `names` found in `json()` to `dst`.
  template<typename Dest>
  void get(Dest& dst, const rajson::Path& path) const
  {
    try {
      json().get(dst, path);
    } catch (const std::exception& e) {
      throw Exception{std::string{"cannot get value of JSON node "}
        .append(path.to_string()).append(": ").append(e.what())};
    }
  }

private:
  rapidjson::Document doc_;
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_DOCUMENT_HPP
