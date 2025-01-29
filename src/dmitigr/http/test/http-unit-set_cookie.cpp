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
#include "../../dt/basics.hpp"
#include "../../http/set_cookie.hpp"

int main()
{
  try {
    namespace dt = dmitigr::dt;
    namespace http = dmitigr::http;
    using http::Set_cookie;
    using dt::Day_of_week;
    using dt::Month;

    {
      const Set_cookie sc{"name", "value"};
      DMITIGR_ASSERT(sc.field_name() == "Set-Cookie");
    }

    {
      const Set_cookie sc{"name", "value"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Expires=Thu, 28 Feb 2019 23:59:59 GMT"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      const auto& ts = sc.expires();
      DMITIGR_ASSERT(ts->day_of_week() == Day_of_week::thu);
      DMITIGR_ASSERT(ts->day() == 28);
      DMITIGR_ASSERT(ts->month() == Month::feb);
      DMITIGR_ASSERT(ts->year() == 2019);
      DMITIGR_ASSERT(ts->hour() == 23);
      DMITIGR_ASSERT(ts->minute() == 59);
      DMITIGR_ASSERT(ts->second() == 59);
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Max-Age=12"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(sc.max_age() == 12);
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Domain=..example.com"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(sc.domain() == "..example.com");
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Path=/path/to/cool/page"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(sc.path() == "/path/to/cool/page");
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Secure"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(sc.is_secure());
      DMITIGR_ASSERT(!sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; HttpOnly"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      DMITIGR_ASSERT(!sc.expires());
      DMITIGR_ASSERT(!sc.max_age());
      DMITIGR_ASSERT(!sc.domain());
      DMITIGR_ASSERT(!sc.path());
      DMITIGR_ASSERT(!sc.is_secure());
      DMITIGR_ASSERT(sc.is_http_only());
    }

    {
      const Set_cookie sc{"name=value; Expires=Thu, 28 Feb 2019 23:59:59 GMT; "
        "Max-Age=12; Domain=..example.com; Path=/path/to/cool/page; Secure; HttpOnly"};
      DMITIGR_ASSERT(sc.name() == "name");
      DMITIGR_ASSERT(sc.value() == "value");
      //
      const auto& ts = sc.expires();
      DMITIGR_ASSERT(ts->day_of_week() == Day_of_week::thu);
      DMITIGR_ASSERT(ts->day() == 28);
      DMITIGR_ASSERT(ts->month() == Month::feb);
      DMITIGR_ASSERT(ts->year() == 2019);
      DMITIGR_ASSERT(ts->hour() == 23);
      DMITIGR_ASSERT(ts->minute() == 59);
      DMITIGR_ASSERT(ts->second() == 59);
      //
      DMITIGR_ASSERT(sc.max_age() == 12);
      DMITIGR_ASSERT(sc.domain() == "..example.com");
      DMITIGR_ASSERT(sc.path() == "/path/to/cool/page");
      DMITIGR_ASSERT(sc.is_secure());
      DMITIGR_ASSERT(sc.is_http_only());
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
