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

#ifndef DMITIGR_RAJSON_CONVERSIONS_HPP
#define DMITIGR_RAJSON_CONVERSIONS_HPP

#include "fwd.hpp" // must be first
#include "../3rdparty/rapidjson/document.h"
#include "../3rdparty/rapidjson/schema.h"
#include "../3rdparty/rapidjson/stringbuffer.h"
#include "../3rdparty/rapidjson/writer.h"
#include "exceptions.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::rajson {

/// The centralized "namespace" for conversion algorithms implementations.
template<typename, typename = void> struct Conversions;

/**
 * @returns The result of conversion of `value` to a JSON text. (Aka
 * serialization.)
 *
 * @throws Exception on error.
 */
template<class Encoding, class Allocator>
std::string to_text(const rapidjson::GenericValue<Encoding, Allocator>& value)
{
  rapidjson::StringBuffer buf;
  rapidjson::Writer<decltype(buf)> writer{buf}; // decltype() required for GCC 7
  if (!value.Accept(writer))
    throw Exception{"cannot convert JSON value to text representation"};
  return std::string{buf.GetString(), buf.GetSize()};
}

/**
 * @returns The result of parsing a JSON text. (Aka deserialization.)
 *
 * @throw Parse_exception on parse error.
 */
inline rapidjson::Document document_from_text(const std::string_view input)
{
  rapidjson::Document result;
  const rapidjson::ParseResult pr{result.Parse(input.data(), input.size())};
  if (!pr)
    throw Parse_exception{pr};
  return result;
}

/// @returns The RapidJSON's string reference.
inline auto to_string_ref(const std::string_view value)
{
  return rapidjson::StringRef(value.data(), value.size());
}

/**
 * @returns The result of conversion of `value` to the value of type `Destination`
 * by using specializations of the template structure Conversions.
 */
template<typename Destination, class Encoding, class Allocator, typename ... Types>
Destination to(const rapidjson::GenericValue<Encoding, Allocator>& value,
  Types&& ... args)
{
  using Dst = std::decay_t<Destination>;
  return Conversions<Dst>::to_type(value, std::forward<Types>(args)...);
}

/// @returns The result of conversion of `value` to the RapidJSON generic value.
template<class Encoding, class Allocator, typename Source, typename ... Types>
rapidjson::GenericValue<Encoding, Allocator> to_value(Source&& value,
  Types&& ... args)
{
  using Src = std::decay_t<Source>;
  return Conversions<Src>::template to_value<Encoding,
    Allocator>(std::forward<Source>(value), std::forward<Types>(args)...);
}

/// @overload
template<typename Source, typename ... Types>
rapidjson::Value to_value(Source&& value, Types&& ... args)
{
  using E = rapidjson::Value::EncodingType;
  using A = rapidjson::Value::AllocatorType;
  return to_value<E, A>(std::forward<Source>(value), std::forward<Types>(args)...);
}

/// @returns The result of conversion of `value` to the RapidJSON generic documnent.
template<class Encoding, class Allocator, typename Source, typename ... Types>
rapidjson::GenericDocument<Encoding, Allocator> to_document(Source&& value,
  Types&& ... args)
{
  rapidjson::GenericDocument<Encoding, Allocator> doc;
  auto val = to_value<Encoding, Allocator>(std::forward<Source>(value),
    doc.GetAllocator(), std::forward<Types>(args)...);
  doc.Swap(val);
  return doc;
}

/// @overload
template<typename Source, typename ... Types>
rapidjson::Document to_document(Source&& value, Types&& ... args)
{
  using E = rapidjson::Value::EncodingType;
  using A = rapidjson::Value::AllocatorType;
  return to_document<E, A>(std::forward<Source>(value), std::forward<Types>(args)...);
}

// -----------------------------------------------------------------------------
// Conversions specializations
// -----------------------------------------------------------------------------

/// Generic implementation of conversion routines for arithmetic types.
struct Arithmetic_generic_conversions {
  template<class Encoding, class Allocator, typename T>
  static auto to_value(const T value)
  {
    static_assert(std::is_arithmetic_v<std::decay_t<T>>);
    return rapidjson::GenericValue<Encoding, Allocator>{value};
  }

  template<class Encoding, class Allocator, typename T>
  static auto to_value(const T value, Allocator&)
  {
    return to_value<Encoding, Allocator>(value);
  }

  template<typename T, class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsInt()) {
      check<T, std::int32_t>();
      const auto result = value.GetInt();
      return static_cast<T>(result);
    } else if (value.IsUint()) {
      check<T, std::uint32_t>();
      const auto result = value.GetUint();
      return static_cast<T>(result);
    } else if (value.IsInt64()) {
      check<T, std::int64_t>();
      const auto result = value.GetInt64();
      return static_cast<T>(result);
    } else if (value.IsUint64()) {
      check<T, std::uint64_t>();
      const auto result = value.GetUint64();
      return static_cast<T>(result);
    } else if (value.IsFloat() || value.IsLosslessFloat()) {
      check<T, float>();
      const auto result = value.GetFloat();
      return static_cast<T>(result);
    } else if (value.IsDouble() || value.IsLosslessDouble()) {
      check<T, double>();
      const auto result = value.GetDouble();
      return static_cast<T>(result);
    }
    throw Exception{"cannot convert JSON value to numeric"};
  }

private:
  template<typename T, typename U>
  static void check()
  {
    if (sizeof(T) < sizeof(U))
      throw Exception{"cannot convert JSON value to numeric:"
        " result size too small"};
  };
};

/// Full specialization for `bool`.
template<>
struct Conversions<bool> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsBool())
      return value.GetBool();

    throw Exception{"cannot convert JSON value to bool"};
  }
};

/// Full specialization for `std::int32_t`.
template<>
struct Conversions<std::int32_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<std::int32_t>(value);
  }
};

/// Full specialization for `std::uint32_t`.
template<>
struct Conversions<std::uint32_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<std::uint32_t>(value);
  }
};

/// Full specialization for `std::int64_t`.
template<>
struct Conversions<std::int64_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<std::int64_t>(value);
  }
};

/// Full specialization for `std::uint64_t`.
template<>
struct Conversions<std::uint64_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<std::uint64_t>(value);
  }
};

/// Full specialization for `float`.
template<>
struct Conversions<float> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<float>(value);
  }
};

/// Full specialization for `double`.
template<>
struct Conversions<double> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return Arithmetic_generic_conversions::to_type<double>(value);
  }
};

/// Partial specialization for `std::chrono::duration<R, P>`.
template<typename Rep, class Period>
struct Conversions<std::chrono::duration<Rep, Period>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    try {
      return std::chrono::duration<Rep, Period>{Conversions<Rep>::to_type(value)};
    } catch (...) {
      throw Exception{"cannot convert JSON value to std::chrono::duration"};
    }
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::chrono::duration<Rep, Period> value, Allocator& alloc)
  {
    return Conversions<Rep>::
      template to_value<Encoding, Allocator>(value.count(), alloc);
  }
};

/// Full specialization for `std::filesystem::path`.
template<>
struct Conversions<std::filesystem::path> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsString())
      return std::filesystem::path{std::string{value.GetString(),
        value.GetStringLength()}};

    throw Exception{"cannot convert JSON value to std::filesystem::path"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::filesystem::path& value, Allocator& alloc)
  {
    // Copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value, alloc};
  }
};

/// Full specialization for `std::string`.
template<>
struct Conversions<std::string> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsString())
      return std::string{value.GetString(), value.GetStringLength()};

    throw Exception{"cannot convert JSON value to std::string"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::string& value, Allocator& alloc)
  {
    // Copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value, alloc};
  }
};

/// Full specialization for `std::string_view`.
template<>
struct Conversions<std::string_view> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsString())
      return std::string_view{value.GetString(), value.GetStringLength()};

    throw Exception{"cannot convert JSON value to std::string_view"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::string_view value)
  {
    // Don't copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value.data(), value.size()};
  }
};

/// Full specialization for `const char*`.
template<>
struct Conversions<const char*> final {
  template<class Encoding, class Allocator>
  static auto to_value(const char* const value, Allocator& alloc)
  {
    // Don't copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value, alloc};
  }
};

/// Partial specialization for enumeration types.
template<typename T>
struct Conversions<T, std::enable_if_t<std::is_enum_v<T>>> {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return static_cast<T>(rajson::to<std::underlying_type_t<T>>(value));
  }

  template<class Encoding, class Allocator>
  static auto to_value(const T value, Allocator&)
  {
    return rapidjson::GenericValue<Encoding, Allocator>(
      static_cast<std::underlying_type_t<T>>(value));
  }
};

/// Partial specialization for `std::optional<T>`.
template<typename T>
struct Conversions<std::optional<T>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return value.IsNull() ?
      std::optional<T>{} :
      std::optional<T>{rajson::to<T>(value)};
  }

  template<class Encoding, class Allocator>
  static auto to_value(std::optional<T>&& value, Allocator& alloc)
  {
    return value ?
      rajson::to_value<Encoding, Allocator>(std::move(*value), alloc) :
      rapidjson::GenericValue<Encoding, Allocator>{};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::optional<T>& value, Allocator& alloc)
  {
    return value ?
      rajson::to_value<Encoding, Allocator>(*value, alloc) :
      rapidjson::GenericValue<Encoding, Allocator>{};
  }
};

/// Partial specialization for `std::vector<T>`.
template<typename T>
struct Conversions<std::vector<T>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsArray()) {
      const auto arr = value.GetArray();
      std::vector<T> result;
      result.reserve(arr.Size());
      for (const auto& val : arr) {
        if (val.IsNull())
          throw Exception{"cannot emplace NULL value to std::vector<T>"};
        result.emplace_back(rajson::to<T>(val));
      }
      return result;
    }

    throw Exception{"cannot convert JSON value to std::vector<T>"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::vector<T>& value, Allocator& alloc)
  {
    rapidjson::GenericValue<Encoding, Allocator> result{rapidjson::kArrayType};
    result.Reserve(value.size(), alloc);
    for (const auto& val : value)
      result.PushBack(rajson::to_value<Encoding, Allocator>(val, alloc), alloc);
    return result;
  }
};

/// Partial specialization for `std::vector<std::optional<T>>`.
template<typename T>
struct Conversions<std::vector<std::optional<T>>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsArray()) {
      const auto arr = value.GetArray();
      std::vector<std::optional<T>> result;
      result.reserve(arr.Size());
      for (const auto& val : arr)
        result.emplace_back(rajson::to<std::optional<T>>(val));
      return result;
    }

    throw Exception{"cannot convert JSON value to std::vector<std::optional<T>>"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::vector<std::optional<T>>& value, Allocator& alloc)
  {
    rapidjson::GenericValue<Encoding, Allocator> result{rapidjson::kArrayType};
    result.Reserve(value.size(), alloc);
    for (const auto& val : value)
      result.PushBack(rajson::to_value<Encoding, Allocator>(val, alloc), alloc);
    return result;
  }
};

/// Partial specialization for `rapidjson::GenericValue`.
template<class Encoding, class Allocator>
struct Conversions<rapidjson::GenericValue<Encoding, Allocator>> final {
  using Type = rapidjson::GenericValue<Encoding, Allocator>;

  template<class E, class A>
  static auto to_type(Type&& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_type(const Type& value, Allocator& alloc)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Type result;
    result.CopyFrom(value, alloc, true);
    return result;
  }

  template<class E, class A>
  static auto to_value(Type&& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_value(Type&& value, Allocator&)
  {
    return to_value<E, A>(std::move(value));
  }

  template<class E, class A>
  static auto to_value(const Type& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return to_type<E, A>(value);
  }

  template<class E, class A>
  static auto to_value(const Type& value, Allocator&)
  {
    return to_value<E, A>(value);
  }
};

/// Partial specialization for `rapidjson::GenericDocument`.
template<class Encoding, class Allocator, class StackAllocator>
struct Conversions<rapidjson::GenericDocument<
                     Encoding, Allocator, StackAllocator>> final {
  using Type = rapidjson::GenericDocument<Encoding, Allocator, StackAllocator>;
  using Value_type = rapidjson::GenericValue<Encoding, Allocator>;

  template<class E, class A>
  static auto to_type(Type&& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_type(Type&& value, Allocator&)
  {
    return to_type<E, A>(std::move(value));
  }

  template<class E, class A>
  static auto to_type(const Type& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Type result;
    result.CopyFrom(value, result.GetAllocator(), true);
    return result;
  }

  template<class E, class A>
  static auto to_type(const Type& value, Allocator&)
  {
    return to_type<E, A>(value);
  }

  template<class E, class A>
  static auto to_value(const Type& value, Allocator& alloc)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Value_type result;
    result.CopyFrom(value, alloc, true);
    return result;
  }
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_CONVERSIONS_HPP
