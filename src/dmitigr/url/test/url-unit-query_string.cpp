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

#include "../../base/assert.hpp"
#include "../../str/transform.hpp"
#include "../../url/query_string.hpp"

int main()
{
  try {
    namespace url = dmitigr::url;

    {
      const url::Query_string qs;
      DMITIGR_ASSERT(qs.parameter_count() == 0);
      DMITIGR_ASSERT(qs.to_string() == "");
    }

    {
      const std::string str{"param1=value1&param2=2"};
      url::Query_string qs{str};
      DMITIGR_ASSERT(qs.to_string() == str);
      DMITIGR_ASSERT(qs.parameter_count() == 2);
      DMITIGR_ASSERT(qs.has_parameter("param1"));
      DMITIGR_ASSERT(qs.has_parameter("param2"));
      DMITIGR_ASSERT(qs.parameter_index("param1").value() == 0);
      DMITIGR_ASSERT(qs.parameter_index("param2").value() == 1);
      DMITIGR_ASSERT(qs.parameter(0).name() == "param1");
      DMITIGR_ASSERT(qs.parameter(1).name() == "param2");
      DMITIGR_ASSERT(qs.parameter(0).value() == "value1");
      DMITIGR_ASSERT(qs.parameter("param1").value() == "value1");
      DMITIGR_ASSERT(qs.parameter(1).value() == "2");
      DMITIGR_ASSERT(qs.parameter("param2").value() == "2");

      qs.append_parameter("param3", "3");
      DMITIGR_ASSERT(qs.parameter_count() == 3);
      DMITIGR_ASSERT(qs.has_parameter("param3"));
      DMITIGR_ASSERT(qs.parameter_index("param3").value() == 2);
      DMITIGR_ASSERT(qs.parameter(2).name() == "param3");
      DMITIGR_ASSERT(qs.parameter(2).value() == "3");
      DMITIGR_ASSERT(qs.parameter("param3").value() == "3");

      qs.parameter(2).set_name("p3");
      DMITIGR_ASSERT(!qs.has_parameter("param3"));
      DMITIGR_ASSERT(qs.has_parameter("p3"));
      DMITIGR_ASSERT(qs.parameter_index("p3").value() == 2);
      DMITIGR_ASSERT(qs.parameter(2).name() == "p3");
      DMITIGR_ASSERT(qs.parameter(2).value() == "3");
      DMITIGR_ASSERT(qs.parameter("p3").value() == "3");

      qs.parameter("p3").set_name("param3");
      DMITIGR_ASSERT(!qs.has_parameter("p3"));
      DMITIGR_ASSERT(qs.has_parameter("param3"));
      DMITIGR_ASSERT(qs.parameter_index("param3").value() == 2);
      DMITIGR_ASSERT(qs.parameter(2).name() == "param3");
      DMITIGR_ASSERT(qs.parameter(2).value() == "3");
      DMITIGR_ASSERT(qs.parameter("param3").value() == "3");

      qs.parameter("param3").set_value("value3");
      DMITIGR_ASSERT(qs.parameter(2).value() == "value3");
      DMITIGR_ASSERT(qs.parameter("param3").value() == "value3");

      qs.remove_parameter("param2");
      DMITIGR_ASSERT(qs.parameter_count() == 2);
      DMITIGR_ASSERT(!qs.has_parameter("param2"));
      DMITIGR_ASSERT(qs.parameter_index("param2") == std::nullopt);
      DMITIGR_ASSERT(qs.parameter(1).name() == "param3");

      qs.remove_parameter(1);
      DMITIGR_ASSERT(qs.parameter_count() == 1);
      DMITIGR_ASSERT(!qs.has_parameter("param3"));
      DMITIGR_ASSERT(qs.parameter_index("param3") == std::nullopt);
      DMITIGR_ASSERT(qs.parameter(0).name() == "param1");
    }

    // -------------------------------------------------------------------------

    {
      using dmitigr::str::to_lowercase;
      const std::string str{"name=%D0%B4%D0%B8%D0%BC%D0%B0&%d0%b2%d0%be%d0%b7%d1%80%d0%b0%d1%81%d1%82=35"};
      url::Query_string qs{str};
      const auto str1 = to_lowercase(str);
      const auto str2 = to_lowercase(qs.to_string());
      DMITIGR_ASSERT(str1 == str2);
      DMITIGR_ASSERT(qs.parameter_count() == 2);
      DMITIGR_ASSERT(qs.has_parameter("name", 0));
      DMITIGR_ASSERT(qs.has_parameter("возраст", 0));
      DMITIGR_ASSERT(qs.parameter_index("name", 0).value() == 0);
      DMITIGR_ASSERT(qs.parameter_index("возраст", 0).value() == 1);
      DMITIGR_ASSERT(qs.parameter(0).name() == "name");
      DMITIGR_ASSERT(qs.parameter(1).name() == "возраст");
      DMITIGR_ASSERT(qs.parameter(0).value() == "дима");
      DMITIGR_ASSERT(qs.parameter("name").value() == "дима");
      DMITIGR_ASSERT(qs.parameter(1).value() == "35");
      DMITIGR_ASSERT(qs.parameter("возраст").value() == "35");
    }

    {
      const std::string str{"name=%D0%B4%D0%B8%D0%BC%D0%B0%20%D0%B8%D0%B3%D1%80%D0%B8%D1%88%D0%B8%D0%BD"};
      const url::Query_string qs{str};
      DMITIGR_ASSERT(qs.to_string() == str);
      DMITIGR_ASSERT(qs.parameter_count() == 1);
      DMITIGR_ASSERT(qs.has_parameter("name", 0));
      DMITIGR_ASSERT(qs.parameter_index("name", 0).value() == 0);
      DMITIGR_ASSERT(qs.parameter(0).name() == "name");
      DMITIGR_ASSERT(qs.parameter(0).value() == "дима игришин");
    }

    {
      const std::string str_plus{"name=%D0%B4%D0%B8%D0%BC%D0%B0+%D0%B8%D0%B3%D1%80%D0%B8%D1%88%D0%B8%D0%BD"};
      const std::string str_20{"name=%D0%B4%D0%B8%D0%BC%D0%B0%20%D0%B8%D0%B3%D1%80%D0%B8%D1%88%D0%B8%D0%BD"};
      const url::Query_string qs{str_plus};
      DMITIGR_ASSERT(qs.to_string() != str_plus); // because space is encoded as %20 rather than as '+'.
      DMITIGR_ASSERT(qs.to_string() == str_20);
      DMITIGR_ASSERT(qs.parameter_count() == 1);
      DMITIGR_ASSERT(qs.has_parameter("name", 0));
      DMITIGR_ASSERT(qs.parameter_index("name", 0).value() == 0);
      DMITIGR_ASSERT(qs.parameter(0).name() == "name");
      DMITIGR_ASSERT(qs.parameter(0).value() == "дима игришин");
    }

    {
      const std::string str{"name=%D0%B4%D0%B8%D0%BC%D0%B0%2B%D0%B8%D0%B3%D1%80%D0%B8%D1%88%D0%B8%D0%BD"};
      const url::Query_string qs{str};
      DMITIGR_ASSERT(qs.to_string() == str);
      DMITIGR_ASSERT(qs.parameter_count() == 1);
      DMITIGR_ASSERT(qs.has_parameter("name", 0));
      DMITIGR_ASSERT(qs.parameter_index("name", 0).value() == 0);
      DMITIGR_ASSERT(qs.parameter(0).name() == "name");
      DMITIGR_ASSERT(qs.parameter(0).value() == "дима+игришин");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
