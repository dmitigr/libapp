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

#ifndef DMITIGR_DT_TIMESTAMP_HPP
#define DMITIGR_DT_TIMESTAMP_HPP

#include "../base/assert.hpp"
#include "basics.hpp"

namespace dmitigr::dt {

/// A timestamp.
class Timestamp final {
public:
  /// @name Constructors
  /// @{

  /// Constructs the timestamp "1583/01/01 00:00:00".
  Timestamp() = default;

  /**
   * @brief Constructs the timestamp by parsing the `input` which is compliant
   * to RFC 7231.
   *
   * @details Examples of valid input:
   *   -# Wed, 06 Apr 1983 17:00:00 GMT.
   *
   * @param input - HTTP date.
   *
   * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Date
   * @see https://tools.ietf.org/html/rfc7231#section-7.1.1.1
   */
  static Timestamp from_rfc7231(const std::string_view input)
  {
    if (input.size() < 29)
      throw Exception{"input doesn't conforms to RFC 7231"};

    Timestamp result;

    static const auto process_integer = [](int& dest,
      const std::string& extracted, const char* const error_message)
    {
      DMITIGR_ASSERT(error_message);
      std::size_t pos{};
      dest = std::stoi(extracted, &pos);
      if (pos != extracted.size())
        throw Exception{error_message};
    };

    /*
     * Extracting the day of week. (Will be checked after extraction of day,
     * month and year)
     */
    Day_of_week dw_extracted{};
    {
      // Note: extracted value is case-sensitive according to RFC7231.
      const std::string_view extracted{input.substr(0, 3)};
      dw_extracted = dt::to_day_of_week(extracted);
    }

    // Extracting the day.
    {
      const char* const errmsg{"day doesn't conforms to RFC 7231"};
      const std::string extracted{input.substr(5, 2)};
      process_integer(result.day_, extracted, errmsg);
      if (result.day_ < 1 || result.day_ > 31)
        throw Exception{errmsg};
    }

    // Extracting the month.
    {
      // Note: extracted value is case-sensitive according to RFC7231.
      const std::string_view extracted{input.substr(8, 3)};
      result.month_ = dt::to_month(extracted);
    }

    // Extracting the year.
    {
      const char* const errmsg{"year doesn't conforms to RFC 7231"};
      const std::string extracted{input.substr(12, 4)};
      process_integer(result.year_, extracted, errmsg);
      if (result.year_ < 1583)
        throw Exception{errmsg};
    }

    // Checking the day of week.
    if (dt::day_of_week(result.year_, result.month_, result.day_) != dw_extracted)
      throw Exception{"day of week doesn't conforms to RFC 7231"};

    // Extracting the hour.
    {
      const char* const errmsg{"hour doesn't conforms to RFC 7231"};
      const std::string extracted{input.substr(17, 2)};
      process_integer(result.hour_, extracted, errmsg);
      if (result.hour_ < 0 || result.hour_ > 23)
        throw Exception{errmsg};
    }

    // Extracting the minute.
    {
      const char* const errmsg{"minute doesn't conforms to RFC 7231"};
      const std::string extracted{input.substr(20, 2)};
      process_integer(result.minute_, extracted, errmsg);
      if (result.minute_ < 0 || result.minute_ > 59)
        throw Exception{errmsg};
    }

    // Extracting the second.
    {
      const char* const errmsg{"second doesn't conforms to RFC 7231"};
      const std::string extracted{input.substr(23, 2)};
      process_integer(result.second_, extracted, errmsg);
      if (result.second_ < 0 || result.second_ > 59)
        throw Exception{errmsg};
    }

    // Checking the timezone.
    {
      // Note: extracted value is case-sensitive according to RFC7231.
      if (input.substr(26, 3) != "GMT")
        throw Exception{"timezone doesn't conforms to RFC 7231"};
    }

    DMITIGR_ASSERT(result.is_invariant_ok());

    return result;
  }

  /// @}

  /// @returns The year.
  int year() const noexcept
  {
    return year_;
  }

  /// @returns The month.
  Month month() const noexcept
  {
    return month_;
  }

  // @returns The day.
  int day() const noexcept
  {
    return day_;
  }

  /// @returns The day of week.
  Day_of_week day_of_week() const
  {
    return dt::day_of_week(year(), month(), day());
  }

  /// @returns The day of year. (Starts at 1.)
  int day_of_year() const
  {
    return dt::day_of_year(year(), month(), day());
  }

  /// @returns The day of epoch (from 1583 Jan 1). (Starts at 1.)
  int day_of_epoch() const
  {
    return dt::day_of_epoch(year(), month(), day());
  }

  /**
   * @brief Sets the date.
   *
   * @par Requires
   * `is_date_acceptable(year, month, day)`.
   */
  void set_date(const int year, const Month month, const int day)
  {
    if (!is_date_acceptable(year, month, day))
      throw Exception{"cannot set non acceptable date to timestamp"};

    year_ = year;
    month_ = month;
    day_ = day;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @par Requires
   * `(day_of_epoch > 0)`.
   */
  void set_date(int day_of_epoch)
  {
    if (!(day_of_epoch > 0))
      throw Exception{"cannot set non positive day of epoch to timestamp"};

    int y = 1583;
    for (int dc = day_count(y); day_of_epoch > dc; dc = day_count(y)) {
      day_of_epoch -= dc;
      ++y;
    }

    DMITIGR_ASSERT(day_of_epoch <= day_count(y));

    int m = static_cast<int>(Month::jan);
    for (int dc = day_count(y, static_cast<Month>(m)); day_of_epoch > dc;
         dc = day_count(y, static_cast<Month>(m))) {
      day_of_epoch -= dc;
      ++m;
    }

    DMITIGR_ASSERT(static_cast<int>(Month::jan) <= m &&
      m <= static_cast<int>(Month::dec));
    DMITIGR_ASSERT(day_of_epoch > 0);

    set_date(y, static_cast<Month>(m), day_of_epoch);
  }

  /// @returns The hour.
  int hour() const noexcept
  {
    return hour_;
  }

  /**
   * @brief Sets the hour.
   *
   * @par Requires
   * `(0 <= hour && hour <= 23)`.
   */
  void set_hour(const int hour)
  {
    if (!(0 <= hour && hour <= 23))
      throw Exception{"cannot set invalid hour to timestamp"};

    hour_ = hour;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The minute.
  int minute() const noexcept
  {
    return minute_;
  }

  /**
   * @brief Sets the minute.
   *
   * @par Requires
   * `(0 <= minute && minute <= 59)`.
   */
  void set_minute(const int minute)
  {
    if (!(0 <= minute && minute <= 59))
      throw Exception{"cannot set invalid minute to timestamp"};

    minute_ = minute;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @returns The second.
  int second() const noexcept
  {
    return second_;
  }

  /**
   * @brief Sets the second.
   *
   * @par Requires
   * `(0 <= second && second <= 59)`.
   */
  void set_second(const int second)
  {
    if (!(0 <= second && second <= 59))
      throw Exception{"cannot set invalid second to timestamp"};

    second_ = second;
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @brief Sets the time.
   *
   * @par Requires
   * `(0 <= hour && hour <= 23) && (0 <= minute && minute <= 59) &&
   * (0 <= second && second <= 59)`.
   */
  void set_time(const int hour, const int minute, const int second)
  {
    set_hour(hour);
    set_minute(minute);
    set_second(second);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @name Conversions
  /// @{

  /**
   * @returns The result of conversion of this instance to the instance of type
   * `std::string` according to RFC7231.
   */
  std::string to_rfc7231() const
  {
    static const auto to_fmt_string = [](const int num)
    {
      return (num < 10) ? std::string{"0"}.append(std::to_string(num)) :
        std::to_string(num);
    };

    std::string result;
    result += dt::to_string_view(day_of_week());
    result += ", ";
    result += to_fmt_string(day_);
    result += " ";
    result += dt::to_string_view(month_);
    result += " ";
    result += to_fmt_string(year_);
    result += " ";
    result += to_fmt_string(hour_);
    result += ":";
    result += to_fmt_string(minute_);
    result += ":";
    result += to_fmt_string(second_);
    result += " GMT";

    return result;
  }

  /// @}

private:
  int day_{1};
  Month month_{Month::jan};
  int year_{1583};
  int hour_{};
  int minute_{};
  int second_{};

  bool is_invariant_ok() const noexcept
  {
    const bool day_ok = (1 <= day() && day() <= day_count(year(), month()));
    const bool year_ok = (1583 <= year());
    const bool hour_ok = (0 <= hour() && hour() <= 23);
    const bool minute_ok = (0 <= minute() && minute() <= 59);
    const bool second_ok = (0 <= second() && second() <= 59);
    return day_ok && year_ok && hour_ok && minute_ok && second_ok;
  }
};

/// @returns `true` if `lhs` is less than `rhs`.
inline bool operator<(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return lhs.year() < rhs.year() ||
    static_cast<int>(lhs.month()) < static_cast<int>(rhs.month()) ||
    lhs.day() < rhs.day() ||
    lhs.hour() < rhs.hour() ||
    lhs.minute() < rhs.minute() ||
    lhs.second() < rhs.second();
}

/// @returns `true` if `lhs` is less than or equal to `rhs`.
inline bool operator<=(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return lhs.year() <= rhs.year() ||
    static_cast<int>(lhs.month()) <= static_cast<int>(rhs.month()) ||
    lhs.day() <= rhs.day() ||
    lhs.hour() <= rhs.hour() ||
    lhs.minute() <= rhs.minute() ||
    lhs.second() <= rhs.second();
}

/// @returns `true` if `lhs` is equal to `rhs`.
inline bool operator==(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return lhs.year() == rhs.year() &&
    lhs.month() == rhs.month() &&
    lhs.day() == rhs.day() &&
    lhs.hour() == rhs.hour() &&
    lhs.minute() == rhs.minute() &&
    lhs.second() == rhs.second();
}

/// @returns `true` if `lhs` is not equal to `rhs`.
inline bool operator!=(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return !(lhs == rhs);
}

/// @returns `true` if `lhs` is greater than `rhs`.
inline bool operator>(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return lhs.year() > rhs.year() ||
    static_cast<int>(lhs.month()) > static_cast<int>(rhs.month()) ||
    lhs.day() > rhs.day() ||
    lhs.hour() > rhs.hour() ||
    lhs.minute() > rhs.minute() ||
    lhs.second() > rhs.second();
}

/// @returns `true` if `lhs` is greater than or equal to `rhs`.
inline bool operator>=(const Timestamp& lhs, const Timestamp& rhs) noexcept
{
  return lhs.year() >= rhs.year() ||
    static_cast<int>(lhs.month()) >= static_cast<int>(rhs.month()) ||
    lhs.day() >= rhs.day() ||
    lhs.hour() >= rhs.hour() ||
    lhs.minute() >= rhs.minute() ||
    lhs.second() >= rhs.second();
}

} // namespace dmitigr::dt

#endif  // DMITIGR_DT_TIMESTAMP_HPP
