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

#ifndef DMITIGR_RAJSON_VALUE_VIEW_HPP
#define DMITIGR_RAJSON_VALUE_VIEW_HPP

#include "conversions.hpp"
#include "exceptions.hpp"
#include "path.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace dmitigr::rajson {

/// A value view.
template<class ValueType>
class Value_view final {
public:
  /// An alias of value type.
  using Value_type = ValueType;

  /// The constructor.
  Value_view(ValueType& value)
    : value_{value}
  {}

  // ---------------------------------------------------------------------------

  /// @returns The JSON value.
  const Value_type& value() const
  {
    return value_;
  }

  /// @overload
  Value_type& value()
  {
    return value_;
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The instance of type `std::optional<Value_view>` bound to member
   * `name`, or `std::nullopt` if either `!value().IsObject()`, or no such a
   * member presents, or if the member is `null`.
   */
  auto optional(const std::string_view name) const
  {
    return optional__(*this, name);
  }

  /// @overload
  auto optional(const std::string_view name)
  {
    return optional__(*this, name);
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The value of member `name` converted to type `R` by using
   * rajson::Conversions, or `std::nullopt` if either `!value().IsObject()`,
   * or no such a member presents, or if the member is `null`.
   */
  template<typename R>
  std::optional<R> optional(const std::string_view name) const
  {
    return optional__<R>(*this, name);
  }

  /// @overload
  template<typename R>
  std::optional<R> optional(const std::string_view name)
  {
    return optional__<R>(*this, name);
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The instance of type `std::optional<Value_view>` bound to path
   * `path`, or `std::nullopt` if either `!value().IsObject()`, or no such a
   * member presents, or if the member is `null`.
   */
  auto optional(const Path& path) const
  {
    return optional__(*this, path);
  }

  /// @overload
  auto optional(const Path& path)
  {
    return optional__(*this, path);
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The value of path `path` converted to type `R` by using
   * rajson::Conversions, or `std::nullopt` if either `!value().IsObject()`,
   * or no such a member presents, or if the member is `null`.
   */
  template<typename R>
  std::optional<R> optional(const Path& path) const
  {
    return optional__<R>(*this, path);
  }

  /// @overload
  template<typename R>
  std::optional<R> optional(const Path& path)
  {
    return optional__<R>(*this, path);
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The instance `std::tuple<std::optional<Value_view>...>` bound to
   * members denoted by `names`. For each name which doesn't corresponds to the
   * member, or for each name which corresponds to the instance of either `null`
   * or not an object, the value of `std::nullopt` is returned.
   */
  template<typename ... String>
  auto optionals(const String& ... names) const
  {
    return optionals__(*this, names...);
  }

  /// @overload
  template<typename ... String>
  auto optionals(const String& ... names)
  {
    return optionals__(*this, names...);
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The instance of type Value_view bound to member denoted by `name`.
   *
   * @par Requires
   * `optional(name)`.
   */
  auto mandatory(const std::string_view name) const
  {
    return mandatory__(*this, name);
  }

  /// @overload
  auto mandatory(const std::string_view name)
  {
    return mandatory__(*this, name);
  }

  /**
   * @returns The value of member `name` converted to type `R` by
   * using rajson::Conversions.
   *
   * @par Requires
   * `optional(name)`.
   */
  template<typename R>
  R mandatory(const std::string_view name) const
  {
    return rajson::to<R>(mandatory(name).value());
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The instance of type Value_view bound to `path`.
   *
   * @par Requires
   * `optional(path)`.
   */
  auto mandatory(const Path& path) const
  {
    return mandatory__(*this, path);
  }

  /// @overload
  auto mandatory(const Path& path)
  {
    return mandatory__(*this, path);
  }

  /**
   * @returns The value of path `path` converted to type `R` by
   * using rajson::Conversions.
   *
   * @par Requires
   * `optional(path)`.
   */
  template<typename R>
  R mandatory(const Path& path) const
  {
    return rajson::to<R>(mandatory(path).value());
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The tuple of instances of Value_view bound to members denoted by
   * `names`.
   */
  template<typename ... String>
  auto mandatories(const String& ... names) const
  {
    return mandatories__(*this, names...);
  }

  /// @overload
  template<typename ... String>
  auto mandatories(const String& ... names)
  {
    return mandatories__(*this, names...);
  }

  // ---------------------------------------------------------------------------

  /// Assigns the value of path `path` found in `json()` to `dst`.
  template<typename Dest>
  void get(std::optional<Dest>& dst, const Path& path) const
  {
    if (auto v = optional<Dest>(path))
      dst = std::move(v);
  }

  /// @overload
  template<typename Dest>
  void get(Dest& dst, const Path& path) const
  {
    dst = mandatory<Dest>(path);
  }

  /// Assigns the value of member `name` found in `json()` to `dst`.
  template<typename Dest>
  void get(std::optional<Dest>& dst, const std::string_view name) const
  {
    if (auto v = optional<Dest>(name))
      dst = std::move(v);
  }

  /// @overload
  template<typename Dest>
  void get(Dest& dst, const std::string_view name) const
  {
    dst = mandatory<Dest>(name);
  }

private:
  Value_type& value_;

  // ---------------------------------------------------------------------------

  template<class Value>
  static auto optional_iterator__(Value& value, const std::string_view name)
  {
    if (const auto e = value.MemberEnd(); name.empty())
      return e;
    else if (const auto m = value.FindMember(rajson::to_string_ref(name)); m != e)
      return m;
    else
      return e;
  }

  // ---------------------------------------------------------------------------

  template<class ValueView>
  static auto optional__(ValueView& view, const std::string_view name)
  {
    /*
     * Note: don't use view.value_ to avoid "value_ is private within this context".
     * This case is possible when accessing the JSON member by path which consists
     * of multiple names. For example, Value_view<Document>::optional__() has
     * no access to Value_view<Value>::value_ when accessing "foo->bar" as
     * Value_view{Document}.mandatory("foo", "bar").
     */
    using Value = decltype(optional_iterator__(view.value(), name)->value);
    using Result = std::conditional_t<
      std::is_const_v<typename std::remove_reference_t<decltype(view.value())>>,
      std::optional<Value_view<std::add_const_t<Value>>>,
      std::optional<Value_view<Value>>
      >;
    if (view.value().IsObject()) {
      if (const auto i = optional_iterator__(view.value(), name);
        i != view.value().MemberEnd() && !i->value.IsNull())
        return Result{i->value};
    }
    return Result{};
  }

  template<typename R, class ValueView>
  static std::optional<R> optional__(ValueView& view, const std::string_view name)
  {
    if (auto result = optional__(view, name))
      return rajson::to<R>(std::move(result->value()));
    else
      return std::nullopt;
  }

  template<class ValueView, class Iter>
  static std::optional<ValueView> optional__(ValueView& view, Iter i, const Iter e)
  {
    auto v = optional__(view, *i);
    return v ? (++i != e ? optional__(*v, i, e) : v) : v;
  }

  template<class ValueView>
  static std::optional<ValueView> optional__(ValueView& view, const Path& path)
  {
    return optional__(view, cbegin(path.components()), cend(path.components()));
  }

  template<typename R, class ValueView>
  static std::optional<R> optional__(ValueView& view, const Path& path)
  {
    if (auto result = optional__(view, path))
      return rajson::to<R>(std::move(result->value()));
    else
      return std::nullopt;
  }

  template<class ValueView, typename ... String>
  static auto optionals__(ValueView& view, const String& ... names)
  {
    return std::make_tuple(optional__(view, names)...);
  }

  // ---------------------------------------------------------------------------

  template<class ValueView>
  static auto mandatory__(ValueView& view, const std::string_view name)
  {
    if (auto result = optional__(view, name))
      return *result;
    else
      throw Exception{std::string{"JSON member \""}.append(name).append("\"")
        .append(" not found")};
  }

  template<class ValueView>
  static auto mandatory__(ValueView& view, const Path& path)
  {
    if (auto result = optional__(view, path))
      return *result;
    else
      throw Exception{std::string{"JSON path \""}.append(path.to_string())
        .append("\"").append(" not found")};
  }

  template<class ValueView, typename ... String>
  static auto mandatories__(ValueView& view, const String& ... names)
  {
    return std::make_tuple(mandatory__(view, names)...);
  }
};

template<class GenericValue> Value_view(GenericValue&) ->
  Value_view<typename GenericValue::ValueType>;

template<class GenericValue> Value_view(const GenericValue&) ->
  Value_view<const typename GenericValue::ValueType>;

} // namespace dmitigr::rajson

#endif // DMITIGR_RAJSON_VALUE_VIEW_HPP
