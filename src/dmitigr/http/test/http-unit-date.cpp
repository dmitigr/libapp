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
#include "../../http/date.hpp"

int main()
{
  try {
    namespace dt = dmitigr::dt;
    namespace http = dmitigr::http;
    using http::Date;
    using dt::Day_of_week;
    using dt::Month;
    using dt::Timestamp;

    {
      const Date d{"Sat, 06 Apr 2019 17:01:02 GMT"};
      DMITIGR_ASSERT(d.field_name() == "Date");
      const auto& ts = d.timestamp();
      DMITIGR_ASSERT(ts.day_of_week() == Day_of_week::sat);
      DMITIGR_ASSERT(ts.day() == 6);
      DMITIGR_ASSERT(ts.month() == Month::apr);
      DMITIGR_ASSERT(ts.year() == 2019);
      DMITIGR_ASSERT(ts.hour() == 17);
      DMITIGR_ASSERT(ts.minute() == 1);
      DMITIGR_ASSERT(ts.second() == 2);
      //
      const auto d_copy = d;
      DMITIGR_ASSERT(d.timestamp() == d_copy.timestamp());
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
