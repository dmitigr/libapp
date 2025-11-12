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
#include "../../base/assoc_vector.hpp"

int main()
{
  try {
    using Avector = dmitigr::Assoc_vector<std::string, std::string>;
    Avector vec;
    DMITIGR_ASSERT(vec.size() == 0);
    DMITIGR_ASSERT(vec.is_empty());
    // Modifying the vec.
    DMITIGR_ASSERT(vec.size() == 0);
    vec.emplace_back("foo", "");
    DMITIGR_ASSERT(vec.size() == 1);
    DMITIGR_ASSERT(!vec.is_empty());
    DMITIGR_ASSERT(vec.vector()[0].first == "foo");
    DMITIGR_ASSERT(vec.index("foo") == 0);
    DMITIGR_ASSERT(vec.value("foo").empty());
    vec.value("foo") = "foo data";
    DMITIGR_ASSERT(!vec.value("foo").empty());
    DMITIGR_ASSERT(vec.value("foo") == "foo data");
    //
    DMITIGR_ASSERT(vec.size() == 1);
    vec.emplace_back("bar", "bar data");
    DMITIGR_ASSERT(vec.size() == 2);
    DMITIGR_ASSERT(!vec.is_empty());
    DMITIGR_ASSERT(vec.vector()[1].first == "bar");
    DMITIGR_ASSERT(vec.index("bar") == 1);
    DMITIGR_ASSERT(!vec.value("bar").empty());
    DMITIGR_ASSERT(vec.value("bar") == "bar data");
    //
    vec.insert(vec.index("bar"), "baz", "1983");
    DMITIGR_ASSERT(vec.size() == 3);
    DMITIGR_ASSERT(!vec.value("baz").empty());
    DMITIGR_ASSERT(vec.value("baz") == "1983");
    vec.remove_each("foo");
    DMITIGR_ASSERT(vec.size() == 2);
    DMITIGR_ASSERT(vec.index("foo") == vec.size());
    vec.remove_each("bar");
    DMITIGR_ASSERT(vec.size() == 1);
    DMITIGR_ASSERT(vec.index("bar") == vec.size());
    DMITIGR_ASSERT(vec.index("baz") != vec.size());

    // -------------------------------------------------------------------------
    // Operators
    // -------------------------------------------------------------------------

    // <, <=
    {
#define ASSERTMENTS                             \
      DMITIGR_ASSERT(lhs < rhs);                \
      DMITIGR_ASSERT(lhs <= rhs);               \
      DMITIGR_ASSERT(!(lhs == rhs));            \
      DMITIGR_ASSERT(lhs != rhs);               \
      DMITIGR_ASSERT(!(lhs > rhs));             \
      DMITIGR_ASSERT(!(lhs >= rhs))

      Avector lhs;
      lhs.emplace_back("name", "dima");
      Avector rhs;
      rhs.emplace_back("name", "olga");
      ASSERTMENTS;
      rhs.value("name") = "olgaolga";
      ASSERTMENTS;
#undef ASSERTMENTS
    }

    // ==, <=, >=
    {
#define ASSERTMENTS                             \
      DMITIGR_ASSERT(!(lhs < rhs));             \
      DMITIGR_ASSERT(lhs <= rhs);               \
      DMITIGR_ASSERT(lhs == rhs);               \
      DMITIGR_ASSERT(!(lhs != rhs));            \
      DMITIGR_ASSERT(!(lhs > rhs));             \
      DMITIGR_ASSERT(lhs >= rhs)

      Avector lhs;
      lhs.emplace_back("name", "dima");
      Avector rhs;
      rhs.emplace_back("name", "dima");
      ASSERTMENTS;
      lhs.value("name") = "";
      rhs.value("name") = "";
      ASSERTMENTS;
#undef ASSERTMENTS
    }

    // >, >=
    {
#define ASSERTMENTS                             \
      DMITIGR_ASSERT(!(lhs < rhs));             \
      DMITIGR_ASSERT(!(lhs <= rhs));            \
      DMITIGR_ASSERT(!(lhs == rhs));            \
      DMITIGR_ASSERT(lhs != rhs);               \
      DMITIGR_ASSERT(lhs > rhs);                \
      DMITIGR_ASSERT(lhs >= rhs)

      Avector lhs;
      lhs.emplace_back("name", "olga");
      Avector rhs;
      rhs.emplace_back("name", "dima");
      ASSERTMENTS;
      lhs.value("name") = "olgaolga";
      ASSERTMENTS;
#undef ASSERTMENTS
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
