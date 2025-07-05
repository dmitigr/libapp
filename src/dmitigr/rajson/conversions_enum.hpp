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

#ifndef DMITIGR_RAJSON_CONVERSIONS_ENUM_HPP
#define DMITIGR_RAJSON_CONVERSIONS_ENUM_HPP

#include "conversions.hpp"

namespace dmitigr::rajson {

/**
 * @brief Traits structure for enumerations.
 *
 * @details Full specialization of Enum_traits must define:
 * @code
 *   template<> struct Enum_traits<Foo> final {
 *     static auto to_type(const std::string_view str) noexcept
 *     {
 *        return to_foo(str);
 *     }
 *     static constexpr const char* singular_name() noexcept
 *     {
 *       return "foo";
 *     }
 *   };
 * @endcode
 */
template<class> struct Enum_traits;

/// Generic enum/JSON conversions.
template<class E>
struct Enum_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    const auto result = Enum_traits<E>::to_type(rajson::to<std::string_view>(value));
    if (!result)
      throw Exception{std::string{"invalid JSON representation of "}
        .append(Enum_traits<E>::singular_name())};
    return *result;
  }

  template<class Encoding, class Allocator>
  static auto to_value(const E value, Allocator&)
  {
    const auto v = to_string_view(value);
    if (v.empty())
      throw Exception{std::string{"cannot represent "}
        .append(Enum_traits<E>::singular_name()).append(" as JSON value")};
    return rajson::to_value(v);
  }
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_CONVERSION_ENUM_HPP
