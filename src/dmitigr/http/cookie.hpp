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

#ifndef DMITIGR_HTTP_COOKIE_HPP
#define DMITIGR_HTTP_COOKIE_HPP

#include "../base/assert.hpp"
#include "exceptions.hpp"
#include "header.hpp"
#include "syntax.hpp"
#include "types_fwd.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace dmitigr::http {

/// A HTTP Cookie header entry.
class Cookie_entry final {
public:
  /**
   * @brief The constructor.
   *
   * @par Requires
   * `is_valid_cookie_name(name) && is_valid_cookie_value(value)`.
   */
  explicit Cookie_entry(std::string name, std::string value = {})
    : name_{std::move(name)}
    , value_{std::move(value)}
  {
    if (!is_valid_cookie_name(name_))
      throw Exception{"cannot create HTTP cookie with invalid name"};
    else if (!is_valid_cookie_value(value_))
      throw Exception{"cannot create HTTP cookie witn invalid value"};

    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The entry name.
  const std::string& name() const noexcept
  {
    return name_;
  }

  /**
   * @brief Sets the name of entry.
   *
   * @par Requires
   * `is_valid_cookie_name(name)`.
   */
  void set_name(std::string name)
  {
    if (!is_valid_cookie_name(name))
      throw Exception{"cannot set invalid name to HTTP cookie"};

    name_ = std::move(name);
  }

  /// @returns The entry value.
  const std::string& value() const noexcept
  {
    return value_;
  }

  /**
   * @brief Sets the value of entry.
   *
   * @par Requires
   * `is_valid_cookie_value(value)`.
   */
  void set_value(std::string value)
  {
    if (!is_valid_cookie_value(value))
      throw Exception{"cannot set invalid value to HTTP cookie"};

    value_ = std::move(value);
  }

private:
  friend Cookie;

  std::string name_;
  std::string value_;

  Cookie_entry() = default; // constructs invalid object!

  bool is_invariant_ok() const
  {
    return is_valid_cookie_name(name_) && is_valid_cookie_value(value_);
  }
};

/**
 * @ingroup headers
 *
 * @brief A HTTP Cookie header.
 *
 * @remarks Since several entries can be named equally, parameter `offset`
 * can be specified as the starting lookup index in the corresponding methods.
 */
class Cookie final : public Header {
public:
  /// Alias of Cookie_entry.
  using Entry = Cookie_entry;

  /**
   * @brief Constructs the object by parsing the `input`.
   *
   * @details Examples of valid input:
   *   -# name=value
   *   -# name=value; name2=value2; name3=value3
   */
  explicit Cookie(const std::string_view input = {})
  {
    /*
     * According to: https://tools.ietf.org/html/rfc6265#section-5.4
     * See also: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cookie
     */

    if (input.empty())
      return;

    enum { name, value, semicolon } state = name;

    const auto append_invalid_entry = [this]{entries_.emplace_back(Entry{});};

    append_invalid_entry();
    std::string* extracted = &entries_.back().name_; // extract name first
    for (const auto c : input) {
      switch (state) {
      case name:
        if (c == '=') {
          extracted = &entries_.back().value_; // extract the value now
          state = value;
          continue; // skip =
        } else if (!detail::rfc6265::is_valid_token_character(c))
          throw Exception{"invalid HTTP cookie name"};
        break;

      case value:
        if (c == ';') {
          DMITIGR_ASSERT(entries_.back().is_invariant_ok()); // check the entry
          append_invalid_entry();
          extracted = &entries_.back().name_; // extract the name now
          state = semicolon;
          continue; // skip ;
        } else if (!detail::rfc6265::is_valid_cookie_octet(c))
          throw Exception{"invalid HTTP cookie value"};
        break;

      case semicolon:
        if (c == ' ') {
          state = name;
          continue; // skip space
        } else
          throw Exception{"no space after the semicolon in HTTP cookie string"};
        break;
      }

      *extracted += c;
    }

    if (state != value)
      throw Exception{"invalid HTTP cookie string"};
  }

  /// @see Header::to_header().
  std::unique_ptr<Header> to_header() const override
  {
    return std::make_unique<Cookie>(*this);
  }

  /// @see Header::field_name().
  const std::string& field_name() const override
  {
    static const std::string result{"Cookie"};
    return result;
  }

  /// @see Header::to_string().
  std::string to_string() const override
  {
    std::string result;
    for (const auto& e : entries_)
      result.append(e.name()).append("=").append(e.value()).append("; ");
    if (!result.empty()) {
      result.pop_back();
      result.pop_back();
    }
    return result;
  }

  /// @returns The number of entries.
  std::size_t entry_count() const
  {
    return entries_.size();
  }

  /**
   * @returns The entry index if `has_entry(name, offset)`, or
   * `std::nullopt` otherwise.
   */
  std::optional<std::size_t> entry_index(const std::string_view name,
    const std::size_t offset = 0) const
  {
    if (offset < entry_count()) {
      const auto b = cbegin(entries_);
      const auto e = cend(entries_);
      const auto i = find_if(b + offset, e, [&](const auto& entry)
      {
        return entry.name() == name;
      });
      if (i != e)
        return std::make_optional(i - b);
    }
    return std::nullopt;
  }

  /**
   * @returns The entry.
   *
   * @par Requires
   * `index < entry_count()`.
   */
  const Entry& entry(const std::size_t index) const
  {
    if (!(index < entry_count()))
      throw Exception{"cannot get HTTP cookie entry by using invalid index"};

    return entries_[index];
  }

  /// @overload
  Entry& entry(const std::size_t index)
  {
    return const_cast<Entry&>(static_cast<const Cookie*>(this)->entry(index));
  }

  /**
   * @overload
   *
   * @par Requires
   * `has_entry(name, offset)`.
   */
  const Entry& entry(const std::string_view name,
    const std::size_t offset = 0) const
  {
    if (const auto index = entry_index(name, offset))
      return entries_[*index];
    else
      throw Exception{std::string{"cannot get HTTP cookie entry by using "
        "invalid name \""}.append(name).append("\"")};
  }

  /// @overload
  Entry& entry(const std::string_view name,
    const std::size_t offset = 0)
  {
    return const_cast<Entry&>(
      static_cast<const Cookie*>(this)->entry(name, offset));
  }

  /// @returns `true` if this instance has the entry with the specified `name`.
  bool has_entry(const std::string_view name, const std::size_t offset = 0) const
  {
    return static_cast<bool>(entry_index(name, offset));
  }

  // @returns `(entry_count() > 0)`.
  bool has_entries() const
  {
    return !entries_.empty();
  }

  /**
   * @brief Appends the entry.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @par Requires
   * `is_valid_cookie_name(name) && is_valid_cookie_value(value)`.
   */
  void append_entry(std::string name, std::string value)
  {
    entries_.emplace_back(std::move(name), std::move(value));
  }

  /**
   * @brief Removes entry.
   *
   * @par Requires
   * `index < entry_count()`.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  void remove_entry(const std::size_t index)
  {
    if (!(index < entry_count()))
      throw Exception{"cannot remove HTTP cookie entry by using invalid index"};

    entries_.erase(cbegin(entries_) + index);
  }

  /**
   * @overload
   *
   * @par Effects
   * `!has_entry(name, offset)`.
   */
  void remove_entry(const std::string_view name, const std::size_t offset = 0)
  {
    if (const auto index = entry_index(name, offset))
      entries_.erase(cbegin(entries_) + *index);

    DMITIGR_ASSERT(!has_entry(name, offset));
  }

private:
  std::vector<Entry> entries_;
};

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_COOKIE_HPP
