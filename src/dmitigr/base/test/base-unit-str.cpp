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
#include "../../base/str.hpp"

int main()
{
  try {
    namespace str = dmitigr::str;
    using namespace std::literals;

    // -------------------------------------------------------------------------
    // trim
    // -------------------------------------------------------------------------

    // Empty string
    {
      auto s = str::trimmed("");
      DMITIGR_ASSERT(s.empty());

      auto sv = str::trimmed(""sv);
      DMITIGR_ASSERT(sv.empty());
    }

    // String with only spaces
    {
      auto s = str::trimmed(" \f\n\r\t\v");
      DMITIGR_ASSERT(s.empty());

      auto sv = str::trimmed(" \f\n\r\t\v"sv);
      DMITIGR_ASSERT(sv.empty());
    }

    // String without spaces
    {
      auto s = str::trimmed("content");
      DMITIGR_ASSERT(s == "content");

      auto sv = str::trimmed("content"sv);
      DMITIGR_ASSERT(sv == "content");
    }

    // String with spaces from left
    {
      auto s = str::trimmed(" \f\n\r\t\vcontent");
      DMITIGR_ASSERT(s == "content");

      auto sv = str::trimmed(" \f\n\r\t\vcontent"sv);
      DMITIGR_ASSERT(sv == "content");
    }

    // String with spaces from right
    {
      auto s = str::trimmed("content \f\n\r\t\v");
      DMITIGR_ASSERT(s == "content");

      auto sv = str::trimmed("content \f\n\r\t\v"sv);
      DMITIGR_ASSERT(sv == "content");
    }

    // String with spaces from both sides
    {
      auto s = str::trimmed(" \f\n\r\t\vcontent \f\n\r\t\v");
      DMITIGR_ASSERT(s == "content");

      auto sv = str::trimmed(" \f\n\r\t\vcontent \f\n\r\t\v"sv);
      DMITIGR_ASSERT(sv == "content");
    }

    // String with spaces from both sides with spaces in the content
    {
      auto s = str::trimmed(" \f\n\r\t\vcon ten t \f\n\r\t\v");
      DMITIGR_ASSERT(s == "con ten t");

      auto sv = str::trimmed(" \f\n\r\t\vcon ten t \f\n\r\t\v"sv);
      DMITIGR_ASSERT(sv == "con ten t");
    }

    // -------------------------------------------------------------------------
    // split
    // -------------------------------------------------------------------------

    // Emptry string, no separators
    {
      std::string s;
      const auto v = str::to_vector(s, "");
      DMITIGR_ASSERT(s.empty());
    }

    // Emptry string and separator
    {
      std::string s;
      const auto v = str::to_vector(s, ",");
      DMITIGR_ASSERT(s.empty());
    }

    // String with only separator
    {
      std::string s{","};
      const auto v = str::to_vector(s, s);
      DMITIGR_ASSERT(v.size() == 2);
    }

    // String with only separators
    {
      std::string s{",,..!!"};
      const auto v = str::to_vector(s, s);
      DMITIGR_ASSERT(v.size() == 7);
    }

    // String without separator
    {
      std::string s{"content"};
      const auto v = str::to_vector(s, ",");
      DMITIGR_ASSERT(v.size() == 1);
    }

    // String with separator
    {
      std::string s{"1 2 3"};
      const auto v = str::to_vector(s, " ");
      DMITIGR_ASSERT(v.size() == 3);
      DMITIGR_ASSERT(v[0] == "1");
      DMITIGR_ASSERT(v[1] == "2");
      DMITIGR_ASSERT(v[2] == "3");
    }

    // String with separators
    {
      std::string s{"1 2,3"};
      const auto v = str::to_vector(s, " ,");
      DMITIGR_ASSERT(v.size() == 3);
      DMITIGR_ASSERT(v[0] == "1");
      DMITIGR_ASSERT(v[1] == "2");
      DMITIGR_ASSERT(v[2] == "3");
    }

    // String with separators to vector of string_view
    {
      std::string s{"1 2,3"};
      const auto v = str::to_vector<std::string_view>(s, " ,");
      DMITIGR_ASSERT(v.size() == 3);
      DMITIGR_ASSERT(v[0] == "1");
      DMITIGR_ASSERT(v[1] == "2");
      DMITIGR_ASSERT(v[2] == "3");
    }

    // -------------------------------------------------------------------------
    // Sparse
    // -------------------------------------------------------------------------

    {
      const char* bytes{""};
      const auto v = str::sparsed_string(std::string_view{bytes, 0},
        str::Byte_format::hex, ":");
      DMITIGR_ASSERT(v.size() == 0);
    }

    {
      char bytes[] = {1,2,3};
      const auto v = str::sparsed_string(std::string_view{bytes, sizeof(bytes)},
        str::Byte_format::hex, ":");
      DMITIGR_ASSERT(v.size() == 3*2 + 3-1);
      DMITIGR_ASSERT(v.substr(0, 2) == "01");
      DMITIGR_ASSERT(v.substr(2, 1) == ":");
      DMITIGR_ASSERT(v.substr(3, 2) == "02");
      DMITIGR_ASSERT(v.substr(5, 1) == ":");
      DMITIGR_ASSERT(v.substr(6, 2) == "03");
      std::cout << v << std::endl;
    }

    {
      char bytes[] = {'d','i','m','a'};
      const auto v = str::sparsed_string(std::string_view{bytes, sizeof(bytes)},
        str::Byte_format::raw, "");
      DMITIGR_ASSERT(v.size() == 4);
      DMITIGR_ASSERT(v[0] == 'd');
      DMITIGR_ASSERT(v[1] == 'i');
      DMITIGR_ASSERT(v[2] == 'm');
      DMITIGR_ASSERT(v[3] == 'a');
    }

    {
      char bytes[] = {'d','i','m','a'};
      const auto v = str::sparsed_string(std::string_view{bytes, sizeof(bytes)},
        str::Byte_format::hex, "");
      DMITIGR_ASSERT(v.size() == 4*2);
      DMITIGR_ASSERT(v == "64696d61");
    }

    // -------------------------------------------------------------------------
    // for_each_part
    // -------------------------------------------------------------------------

    for_each_part([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "1");
      DMITIGR_ASSERT(i != 1 || part == "23");
      DMITIGR_ASSERT(i != 2 || part == "456");
      DMITIGR_ASSERT(0 <= i && i <= 2);
      ++i;
      return true;
    }, std::string{"1,23,456"}, str::Fepsep_all{","});

    for_each_part([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "1");
      DMITIGR_ASSERT(i != 1 || part == "23");
      DMITIGR_ASSERT(i != 2 || part == "456");
      DMITIGR_ASSERT(0 <= i && i <= 2);
      ++i;
      return true;
    }, "1-~-23-~-456", str::Fepsep_all{"-~-"});

    for_each_part_backward([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "456");
      DMITIGR_ASSERT(i != 1 || part == "23");
      DMITIGR_ASSERT(i != 2 || part == "1");
      DMITIGR_ASSERT(0 <= i && i <= 2);
      ++i;
      return true;
    }, std::string{"1,23,456"}, str::Fepsep_all{","});

    for_each_part_backward([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "456");
      DMITIGR_ASSERT(i != 1 || part == "23");
      DMITIGR_ASSERT(i != 2 || part == "1");
      DMITIGR_ASSERT(0 <= i && i <= 2);
      ++i;
      return true;
    }, "1-~-23-~-456", str::Fepsep_all{"-~-"});

    for_each_part([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "1");
      DMITIGR_ASSERT(i != 1 || part == "23");
      DMITIGR_ASSERT(i != 2 || part == "456");
      DMITIGR_ASSERT(0 <= i && i <= 2);
      ++i;
      return true;
    }, "1\r\n\r\n23\n\n\n\r456\n", str::Fepsep_any{"\r\n"});

    for_each_part_backward([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "");
      DMITIGR_ASSERT(i != 1 || part == "456");
      DMITIGR_ASSERT(i != 2 || part == "23");
      DMITIGR_ASSERT(i != 3 || part == "1");
      DMITIGR_ASSERT(0 <= i && i <= 3);
      ++i;
      return true;
    }, "1\r\n\r\n23\n\n\n\r456\n", str::Fepsep_any{"\r\n"});

    for_each_part([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "");
      DMITIGR_ASSERT(i != 1 || part == "abc");
      DMITIGR_ASSERT(i != 2 || part == "bc");
      DMITIGR_ASSERT(i != 3 || part == "b");
      DMITIGR_ASSERT(0 <= i && i <= 3);
      ++i;
      return true;
    }, "1abc23bc456b", str::Fepsep_none{"abc"});

    for_each_part_backward([i=0](const auto part)mutable
    {
      DMITIGR_ASSERT(i != 0 || part == "b");
      DMITIGR_ASSERT(i != 1 || part == "bc");
      DMITIGR_ASSERT(i != 2 || part == "abc");
      DMITIGR_ASSERT(i != 3 || part == "");
      DMITIGR_ASSERT(0 <= i && i <= 3);
      ++i;
      return true;
    }, "1abc23bc456b", str::Fepsep_none{"abc"});
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
