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
#include "../../http/cookie.hpp"

int main()
{
  try {
    namespace http = dmitigr::http;
    using http::Cookie;
    {
      Cookie c;
      DMITIGR_ASSERT(c.entry_count() == 0);
      DMITIGR_ASSERT(c.field_name() == "Cookie");
      DMITIGR_ASSERT(!c.has_entries());

      const auto c_copy = c;
      DMITIGR_ASSERT(c_copy.entry_count() == 0);
      DMITIGR_ASSERT(!c_copy.has_entries());

      c.append_entry("name", "value");
      DMITIGR_ASSERT(c.entry_count() == 1);
      DMITIGR_ASSERT(c.entry_index("name") == 0);
      DMITIGR_ASSERT(c.entry(0).name() == "name");
      DMITIGR_ASSERT(c.entry(0).value() == "value");
      DMITIGR_ASSERT(c.entry("name").name() == "name");
      DMITIGR_ASSERT(c.entry("name").value() == "value");
      DMITIGR_ASSERT(c.has_entry("name"));
      DMITIGR_ASSERT(c.has_entries());

      c.remove_entry("name");
      DMITIGR_ASSERT(c.entry_count() == 0);
      DMITIGR_ASSERT(!c.has_entries());

      c.append_entry("name", "value");
      c.remove_entry(0);
      DMITIGR_ASSERT(c.entry_count() == 0);
      DMITIGR_ASSERT(!c.has_entries());

      c.append_entry("name", "value");
      c.entry(0).set_name("another_name");
      DMITIGR_ASSERT(c.entry_index("another_name") == 0);
      DMITIGR_ASSERT(c.entry(0).name() == "another_name");

      c.entry("another_name").set_name("name");
      DMITIGR_ASSERT(c.entry_index("name") == 0);
      DMITIGR_ASSERT(c.entry(0).name() == "name");

      c.entry("name").set_value("another_value");
      DMITIGR_ASSERT(c.entry("name").value() == "another_value");
      DMITIGR_ASSERT(c.entry(0).name() == "name");
    }

    {
      const Cookie c{"name=value"};
      DMITIGR_ASSERT(c.entry_count() == 1);
      DMITIGR_ASSERT(c.entry_index("name") == 0);
      DMITIGR_ASSERT(c.entry(0).name() == "name");
      DMITIGR_ASSERT(c.entry(0).value() == "value");
      DMITIGR_ASSERT(c.entry("name").value() == "value");
      DMITIGR_ASSERT(c.has_entry("name"));
      DMITIGR_ASSERT(c.has_entries());
    }

    {
      const Cookie c{"name=value; name2=value2; name3=value3"};
      DMITIGR_ASSERT(c.entry_count() == 3);
      DMITIGR_ASSERT(c.has_entries());
      //
      DMITIGR_ASSERT(c.entry_index("name") == 0);
      DMITIGR_ASSERT(c.entry(0).name() == "name");
      DMITIGR_ASSERT(c.entry(0).value() == "value");
      DMITIGR_ASSERT(c.entry("name").name() == "name");
      DMITIGR_ASSERT(c.entry("name").value() == "value");
      DMITIGR_ASSERT(c.has_entry("name"));
      //
      DMITIGR_ASSERT(c.entry_index("name2") == 1);
      DMITIGR_ASSERT(c.entry(1).name() == "name2");
      DMITIGR_ASSERT(c.entry(1).value() == "value2");
      DMITIGR_ASSERT(c.entry("name2").name() == "name2");
      DMITIGR_ASSERT(c.entry("name2").value() == "value2");
      DMITIGR_ASSERT(c.has_entry("name2"));
      //
      DMITIGR_ASSERT(c.entry_index("name3") == 2);
      DMITIGR_ASSERT(c.entry(2).name() == "name3");
      DMITIGR_ASSERT(c.entry(2).value() == "value3");
      DMITIGR_ASSERT(c.entry("name3").name() == "name3");
      DMITIGR_ASSERT(c.entry("name3").value() == "value3");
      DMITIGR_ASSERT(c.has_entry("name3"));
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
