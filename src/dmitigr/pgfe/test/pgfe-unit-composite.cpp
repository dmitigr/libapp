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
#include "../../pgfe/conversions.hpp"
#include "../../pgfe/data.hpp"
#include "../../pgfe/tuple.hpp"

int main()
{
  try {
    namespace pgfe = dmitigr::pgfe;
    pgfe::Tuple tuple;
    DMITIGR_ASSERT(tuple.field_count() == 0);
    DMITIGR_ASSERT(tuple.is_empty());
    // Modifying the tuple.
    DMITIGR_ASSERT(tuple.field_count() == 0);
    tuple.append("foo", nullptr);
    DMITIGR_ASSERT(tuple.field_count() == 1);
    DMITIGR_ASSERT(!tuple.is_empty());
    DMITIGR_ASSERT(tuple.field_name(0) == "foo");
    DMITIGR_ASSERT(tuple.field_index("foo") == 0);
    DMITIGR_ASSERT(!tuple.data(0));
    DMITIGR_ASSERT(!tuple.data("foo"));
    tuple.set("foo", "foo data");
    DMITIGR_ASSERT(tuple.data(0));
    DMITIGR_ASSERT(tuple.data("foo"));
    DMITIGR_ASSERT(pgfe::to<std::string_view>(tuple[0]) == "foo data");
    DMITIGR_ASSERT(pgfe::to<std::string_view>(tuple["foo"]) == "foo data");
    //
    DMITIGR_ASSERT(tuple.field_count() == 1);
    tuple.append("bar", "bar data");
    DMITIGR_ASSERT(tuple.field_count() == 2);
    DMITIGR_ASSERT(!tuple.is_empty());
    DMITIGR_ASSERT(tuple.field_name(1) == "bar");
    DMITIGR_ASSERT(tuple.field_index("bar") == 1);
    DMITIGR_ASSERT(tuple.data(1));
    DMITIGR_ASSERT(tuple.data("bar"));
    DMITIGR_ASSERT(pgfe::to<std::string_view>(tuple.data(1)) == "bar data");
    DMITIGR_ASSERT(pgfe::to<std::string_view>(tuple.data("bar")) == "bar data");
    //
    tuple.insert("bar", "baz", 1983);
    DMITIGR_ASSERT(tuple.field_count() == 3);
    DMITIGR_ASSERT(tuple.data(2));
    DMITIGR_ASSERT(tuple.data("baz"));
    DMITIGR_ASSERT(pgfe::to<int>(tuple.data("baz")) == 1983);
    tuple.remove("foo");
    DMITIGR_ASSERT(tuple.field_count() == 2);
    DMITIGR_ASSERT(tuple.field_index("foo") == tuple.field_count());
    tuple.remove("bar");
    DMITIGR_ASSERT(tuple.field_count() == 1);
    DMITIGR_ASSERT(tuple.field_index("bar") == tuple.field_count());
    DMITIGR_ASSERT(tuple.field_index("baz") != tuple.field_count());

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

      pgfe::Tuple lhs;
      lhs.append("name", "dima");
      pgfe::Tuple rhs;
      rhs.append("name", "olga");
      ASSERTMENTS;
      rhs.set("name", pgfe::Data::make("olgaolga"));
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

      pgfe::Tuple lhs;
      lhs.append("name", "dima");
      pgfe::Tuple rhs;
      rhs.append("name", "dima");
      ASSERTMENTS;
      lhs.set("name", "");
      rhs.set("name", "");
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

      pgfe::Tuple lhs;
      lhs.append("name", "olga");
      pgfe::Tuple rhs;
      rhs.append("name", "dima");
      ASSERTMENTS;
      lhs.set("name", "olgaolga");
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
