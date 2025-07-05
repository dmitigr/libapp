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

#ifndef DMITIGR_HTTP_SET_COOKIE_HPP
#define DMITIGR_HTTP_SET_COOKIE_HPP

#include "../base/assert.hpp"
#include "../dt/timestamp.hpp"
#include "../net/util.hpp"
#include "../str/predicate.hpp"
#include "../str/transform.hpp"
#include "basics.hpp"
#include "exceptions.hpp"
#include "header.hpp"
#include "syntax.hpp"

#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>

namespace dmitigr::http {

/**
 * @ingroup headers
 *
 * @brief Defines an abstraction of the HTTP Set-Cookie header.
 */
class Set_cookie final : public Header {
public:
  /**
   * @brief Constructs the object by parsing the `input`.
   *
   * Examples of valid input are:
   *
   * name=value
   * name=value; Expires=Sat, 06 Jul 2019 13:23:00 GMT
   * name=value; Max-Age=60
   * name=value; Domain=example.com
   * name=value; Path=/docs/web
   * name=value; Secure
   * name=value; HttpOnly
   * name=value; SameSite=Strict
   * name=value; SameSite=Lax
   * name=value; Domain=example.com; Secure; HttpOnly
   *
   * @param input - set-cookie-string.
   *
   * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
   */
  explicit Set_cookie(const std::string_view input)
  {
    // According to: https://tools.ietf.org/html/rfc6265#section-4.1

    enum { name, before_value, value_quoted, value_unquoted,
      after_value_quoted, semicolon, attr_name, attr_value } state = name;

    static const auto is_valid_attr_name_character = [](const unsigned char c)
    {
      return std::isalpha(c) || c == '-';
    };

    static const auto is_valid_attr_value_character = [](const char c)
    {
      return !detail::is_ctl(c) && c != ';';
    };

    const auto store_name = [this](std::string& str)
    {
      name_ = std::move(str);
      str = {};
    };

    const auto store_value = [this](std::string& str)
    {
      value_ = std::move(str);
      str = {};
    };

    std::string attr_type;
    const auto process_boolean_attr_type = [&](std::string& str)
    {
      str::lowercase(str);
      if (str == "secure")
        is_secure_ = true;
      else if (str == "httponly")
        is_http_only_ = true;
      else
        throw Exception{"invalid HTTP Set-Cookie cookie attribute name"};
      str = {};
    };
    const auto set_attr_type = [&attr_type](std::string& str)
    {
      str::lowercase(str);
      attr_type = std::move(str);
      str = {};
    };

    const auto store_attr_value = [this, &attr_type](std::string& str)
    {
      if (str.empty())
        throw Exception{"empty values of HTTP Set-Cookie cookie attributes are "
          "not allowed"};

      DMITIGR_ASSERT(attr_type != "secure" && attr_type != "httponly");
      if (attr_type == "expires") {
        expires_ = dt::Timestamp::from_rfc7231(str);
      } else if (attr_type == "max-age") {
        std::size_t pos{};
        max_age_ = std::stoi(str, &pos);
        if (pos != str.size())
          throw Exception{"invalid value of HTTP Set-Cookie cookie Max-Age "
            "attribute"};
      } else if (attr_type == "domain") {
        const auto pos = str.find_first_not_of('.'); // ignore leading dots
        if (pos != std::string::npos) {
          const std::string host{str.data() + pos, str.size() - pos};
          if (!net::is_hostname_valid(host))
            throw Exception{"invalid value of HTTP Set-Cookie cookie Domain "
              "attribute"};
        }
        domain_ = std::move(str);
      } else if (attr_type == "path") {
        if (str.front() != '/')
          throw Exception{"invalid value of HTTP Set-Cookie cookie Path "
            "attribute"};
        path_ = std::move(str);
      } else if (attr_type == "samesite") {
        str::lowercase(str);
        if (str == "strict")
          same_site_ = Same_site::strict;
        else if (str == "lax")
          same_site_ = Same_site::lax;
        else
          throw Exception{"invalid value of HTTP Set-Cookie cookie SameSite "
            "attribute"};
      } else
        throw Exception{"unknown attribute name of HTTP Set-Cookie cookie"};

      str = {};
    };

    std::string extracted;
    for (const auto c : input) {
      switch (state) {
      case name:
        if (c == '=') {
          store_name(extracted);
          state = before_value;
          continue; // skip =
        } else if (!detail::rfc6265::is_valid_token_character(c))
          throw Exception{"invalid HTTP Set-Cookie cookie name"};
        break;

      case before_value:
        if (c == ';') {
          // Empty value is okay.
          state = semicolon;
          continue; // skip ;
        } else if (c == '"') {
          state = value_quoted;
          continue; // skip "
        } else if (detail::rfc6265::is_valid_token_character(c)) {
          state = value_unquoted;
        } else
          throw Exception{"invalid HTTP Set-Cookie cookie value"};
        break;

      case value_quoted:
        if (c == '"') {
          state = after_value_quoted;
          continue; // skip "
        } else if (!detail::rfc6265::is_valid_cookie_octet(c))
          throw Exception{"invalid HTTP Set-Cookie cookie value"};
        break;

      case value_unquoted:
        if (c == ';') {
          store_value(extracted);
          state = semicolon;
          continue; // skip ;
        } else if (!detail::rfc6265::is_valid_cookie_octet(c))
          throw Exception{"invalid HTTP Set-Cookie cookie value"};
        break;

      case after_value_quoted:
        if (c == ';') {
          store_value(extracted);
          state = semicolon;
          continue; // skip ;
        } else
          throw Exception{"no semicolon after quoted HTTP Set-Cookie cookie "
            "value"};
        break;

      case semicolon:
        if (c == ' ') {
          state = attr_name;
          continue; // skip space
        } else
          throw Exception{"no space after semicolon in HTTP Set-Cookie cookie "
            "string"};
        break;

      case attr_name:
        if (c == ';') {
          process_boolean_attr_type(extracted);
          state = semicolon;
          continue; // skip ;
        } else if (c == '=') {
          set_attr_type(extracted);
          state = attr_value;
          continue; // skip =
        } else if (!is_valid_attr_name_character(c))
          throw Exception{"invalid attribute name in HTTP Set-Cookie cookie"};
        [[fallthrough]];

      case attr_value:
        if (c == ';') {
          store_attr_value(extracted);
          state = semicolon;
          continue; // skip ;
        } else if (!is_valid_attr_value_character(c))
          throw Exception{"invalid attribute value in HTTP Set-Cookie cookie"};
      }

      extracted += c;
    }

    switch (state) {
    case value_unquoted:
      [[fallthrough]];
    case after_value_quoted:
      store_value(extracted);
      break;
    case attr_name:
      process_boolean_attr_type(extracted);
      break;
    case attr_value:
      store_attr_value(extracted);
      break;
    case name:
      [[fallthrough]];
    case before_value:
      [[fallthrough]];
    case value_quoted:
      [[fallthrough]];
    case semicolon:
      throw Exception{"invalid HTTP Set-Cookie string"};
    }

    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @par Requires
   * `(is_valid_cookie_name(name) && is_valid_cookie_value(value))`.
   */
  Set_cookie(std::string name, std::string value)
    : name_{std::move(name)}
    , value_{std::move(value)}
  {
    if (!is_valid_cookie_name(name_))
      throw Exception{"cannot create Set-Cookie HTTP cookie with invalid name"};
    else if (!is_valid_cookie_value(value_))
      throw Exception{"cannot create Set-Cookie HTTP cookie with invalid value"};

    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// Swaps this instance with `other`.
  void swap(Set_cookie& other) noexcept
  {
    using std::swap;
    swap(name_, other.name_);
    swap(value_, other.value_);
    swap(expires_, other.expires_);
    swap(max_age_, other.max_age_);
    swap(domain_, other.domain_);
    swap(path_, other.path_);
    swap(is_secure_, other.is_secure_);
    swap(is_http_only_, other.is_http_only_);
    swap(same_site_, other.same_site_);
  }

  /// @see Header::to_header().
  std::unique_ptr<Header> to_header() const override
  {
    return std::make_unique<Set_cookie>(*this);
  }

  /// @see Header::field_name().
  const std::string& field_name() const override
  {
    static const std::string result{"Set-Cookie"};
    return result;
  }

  /// @see Header::to_string().
  std::string to_string() const override
  {
    std::string result;
    result.append(name_).append("=").append(value_);
    if (expires_)
      result.append("; ").append("Expires=").append(expires_->to_rfc7231());
    if (max_age_)
      result.append("; ").append("Max-Age=").append(std::to_string(*max_age_));
    if (domain_)
      result.append("; ").append("Domain=").append(*domain_);
    if (path_)
      result.append("; ").append("Path=").append(*path_);
    if (is_secure_)
      result.append("; ").append("Secure");
    if (is_http_only_)
      result.append("; ").append("HttpOnly");
    if (same_site_)
      result.append("; ").append("SameSite=")
        .append(http::to_string_view(*same_site_));
    return result;
  }

  /**
   * @returns The name of cookie.
   *
   * @see set_name().
   */
  const std::string& name() const noexcept
  {
    return name_;
  }

  /**
   * @brief Sets the name of cookie.
   *
   * @par Requires
   * `is_valid_cookie_name(name)`.
   *
   * @see name().
   */
  void set_name(std::string name)
  {
    if (!is_valid_cookie_name(name))
      throw Exception{"cannot set invalid name to Set-Cookie HTTP cookie"};

    check_consistency(name, is_secure_, domain_, path_);
    name_ = std::move(name);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The value of cookie.
   *
   * @see set_value().
   */
  const std::string& value() const noexcept
  {
    return value_;
  }

  /**
   * @brief Sets the value of cookie.
   *
   * @par Requires
   * `is_valid_cookie_value(value)`.
   *
   * @see value().
   */
  void set_value(std::string value)
  {
    if (!is_valid_cookie_value(value))
      throw Exception{"cannot set invalid value to Set-Cookie HTTP cookie"};

    value_ = std::move(value);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The value of "Expires" attribute of cookie.
   *
   * @see set_expires().
   */
  const std::optional<dt::Timestamp>& expires() const noexcept
  {
    return expires_;
  }

  /// Sets the "Expires" attribute of cookie.
  void set_expires(std::optional<dt::Timestamp> ts)
  {
    expires_ = std::move(ts);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @see Date.
   */
  void set_expires(const std::string_view input)
  {
    expires_ = dt::Timestamp::from_rfc7231(input);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The value of "MaxAge" attribute of cookie.
  std::optional<int> max_age() const noexcept
  {
    return max_age_;
  }

  /// Sets the "Max-Age" attribute of cookie.
  void set_max_age(std::optional<int> ma)
  {
    max_age_ = ma;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The value of "Domain" attribute of cookie.
  const std::optional<std::string>& domain() const noexcept
  {
    return domain_;
  }

  /// Sets the "Domain" attribute of cookie.
  void set_domain(std::optional<std::string> domain)
  {
    check_consistency(name_, is_secure_, domain, path_);
    domain_ = std::move(domain);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The value of "Path" attribute of cookie.
  const std::optional<std::string>& path() const noexcept
  {
    return path_;
  }

  /// Sets the "Path" attribute of cookie.
  void set_path(std::optional<std::string> path)
  {
    check_consistency(name_, is_secure_, domain_, path);
    path_ = std::move(path);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns `true` if the "Secure" attribute is presents in cookie.
  bool is_secure() const noexcept
  {
    return is_secure_;
  }

  /// Sets the "Secure" attribute of cookie.
  void set_secure(const bool secure)
  {
    check_consistency(name_, secure, domain_, path_);
    is_secure_ = secure;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns `true` if the "HttpOnly" attribute is presents in cookie.
  bool is_http_only() const noexcept
  {
    return is_http_only_;
  }

  /// Sets the "HttpOnly" attribute of cookie.
  void set_http_only(const bool http_only)
  {
    is_http_only_ = http_only;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The value of "SameSite" attribute of cookie.
  std::optional<Same_site> same_site() const noexcept
  {
    return same_site_;
  }

  /// Sets the "SameSite" attribute of cookie.
  void set_same_site(std::optional<Same_site> same_site)
  {
    same_site_ = std::move(same_site);
    DMITIGR_ASSERT(is_invariant_ok());
  }

private:
  std::string name_;
  std::string value_;
  std::optional<dt::Timestamp> expires_;
  std::optional<int> max_age_{};
  std::optional<std::string> domain_;
  std::optional<std::string> path_;
  bool is_secure_{};
  bool is_http_only_{};
  std::optional<Same_site> same_site_{};

  // ===========================================================================

  bool is_invariant_ok() const
  {
    return is_valid_cookie_name(name_) && is_valid_cookie_value(value_) &&
      !requirement_violation_details(name_, is_secure_, domain_, path_);
  }

  // ===========================================================================

  /// @returns The string literal with a requirement violation info.
  static const char* requirement_violation_details(const std::string& name,
    const bool is_secure,
    const std::optional<std::string>& domain,
    const std::optional<std::string>& path)
  {
    if (str::is_begins_with(name, "__Secure-")) {
      if (!is_secure)
        return "cookies with name starting __Secure- must be set with "
          "\"secure\" flag";
    }

    if (str::is_begins_with(name, "__Host-")) {
      if (!is_secure)
        return "cookies with name starting __Host- must be set with "
          "\"secure\" flag";
      if (domain)
        return "cookies with name starting __Host- must not have a "
          "domain specified";
      if (path != "/")
        return "cookies with name starting __Host- must have path \"/\"";
    }

    return nullptr;
  }

  /**
   * @brief Checks the consistency of the arguments.
   *
   * @throws Exception if the specified arguments are not consistent.
   */
  static void check_consistency(const std::string& name,
    const bool is_secure,
    const std::optional<std::string>& domain,
    const std::optional<std::string>& path)
  {
    const char* const details = requirement_violation_details(name, is_secure,
      domain, path);
    if (details)
      throw Exception{details};
  }
};

} // namespace dmitigr::http

#endif  // DMITIGR_HTTP_SET_COOKIE_HPP
