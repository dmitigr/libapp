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

#include "../../base/assert.hpp"
#include "../../dt/timestamp.hpp"

int main()
{
  try {
    namespace dt = dmitigr::dt;
    using dt::Timestamp;
    using dt::Month;
    using dt::Day_of_week;

    Timestamp ts;
    DMITIGR_ASSERT(ts.year() == 1583);
    DMITIGR_ASSERT(ts.month() == Month::jan);
    DMITIGR_ASSERT(ts.day() == 1);
    DMITIGR_ASSERT(ts.hour() == 0);
    DMITIGR_ASSERT(ts.minute() == 0);
    DMITIGR_ASSERT(ts.second() == 0);
    DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::sat);
    DMITIGR_ASSERT(ts.day_of_year() == 1);
    DMITIGR_ASSERT(ts.day_of_epoch() == 1);

    ts.set_date(1);
    DMITIGR_ASSERT(ts.year() == 1583);
    DMITIGR_ASSERT(ts.month() == Month::jan);
    DMITIGR_ASSERT(ts.day() == 1);

    ts.set_date(365);
    DMITIGR_ASSERT(ts.year() == 1583);
    DMITIGR_ASSERT(ts.month() == Month::dec);
    DMITIGR_ASSERT(ts.day() == 31);

    ts.set_date(365 + 1);
    DMITIGR_ASSERT(ts.year() == 1584);
    DMITIGR_ASSERT(ts.month() == Month::jan);
    DMITIGR_ASSERT(ts.day() == 1);

    ts.set_date(365 + 31 + 29);
    DMITIGR_ASSERT(ts.year() == 1584);
    DMITIGR_ASSERT(ts.month() == Month::feb);
    DMITIGR_ASSERT(ts.day() == 29);

    ts.set_date(1988, Month::oct, 26);
    ts.set_time(19, 39, 59);
    DMITIGR_ASSERT(ts.year() == 1988);
    DMITIGR_ASSERT(ts.month() == Month::oct);
    DMITIGR_ASSERT(ts.day() == 26);
    DMITIGR_ASSERT(ts.hour() == 19);
    DMITIGR_ASSERT(ts.minute() == 39);
    DMITIGR_ASSERT(ts.second() == 59);
    DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::wed);
    DMITIGR_ASSERT(ts.day_of_year() == 300);
    DMITIGR_ASSERT(ts.day_of_epoch() == 148223);

    ts.set_date(148223);
    DMITIGR_ASSERT(ts.year() == 1988);
    DMITIGR_ASSERT(ts.month() == Month::oct);
    DMITIGR_ASSERT(ts.day() == 26);
    DMITIGR_ASSERT(ts.hour() == 19);
    DMITIGR_ASSERT(ts.minute() == 39);
    DMITIGR_ASSERT(ts.second() == 59);
    DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::wed);
    DMITIGR_ASSERT(ts.day_of_year() == 300);
    DMITIGR_ASSERT(ts.day_of_epoch() == 148223);

    {
      std::string ts_str{"Wed, 06 Apr 1983 17:00:00 GMT"};
      ts = Timestamp::from_rfc7231(ts_str);
      DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::wed);
      DMITIGR_ASSERT(ts.day() == 6);
      DMITIGR_ASSERT(ts.month() == Month::apr);
      DMITIGR_ASSERT(ts.year() == 1983);
      DMITIGR_ASSERT(ts.hour() == 17);
      DMITIGR_ASSERT(ts.minute() == 0);
      DMITIGR_ASSERT(ts.second() == 0);
      DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::wed);
      DMITIGR_ASSERT(ts.day_of_year() == 96);
      DMITIGR_ASSERT(ts.day_of_epoch() == 146193);
      DMITIGR_ASSERT(ts.to_rfc7231() == ts_str);
    }

    {
      Timestamp ts1;
      Timestamp ts2;
      DMITIGR_ASSERT(ts1 == ts2);

      ts2.set_hour(1);
      DMITIGR_ASSERT(ts1 < ts2);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
