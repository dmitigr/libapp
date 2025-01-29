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

#include "pgfe-unit.hpp"

int main() try {
  namespace pgfe = dmitigr::pgfe;
  using pgfe::a;
  using pgfe::to;
  using pgfe::Data_view;

  // Basic tests.
  {
    a na1{"null", nullptr};
    DMITIGR_ASSERT(na1.name() == "null");
    DMITIGR_ASSERT(!na1.data());

    auto data = pgfe::to_data(1);
    DMITIGR_ASSERT(data);

    a na2{"without-ownership", *data};
    DMITIGR_ASSERT(na2.name() == "without-ownership");
    DMITIGR_ASSERT(na2.data() == data.get());
    DMITIGR_ASSERT(!na2.owns_data());

    const auto* const data_ptr = data.get();
    DMITIGR_ASSERT(data_ptr);
    a na3{"with-ownership", std::move(data)};
    DMITIGR_ASSERT(na3.name() == "with-ownership");
    DMITIGR_ASSERT(!data);
    DMITIGR_ASSERT(na3.data() == data_ptr);
    DMITIGR_ASSERT(na3.owns_data());

    a na4{"ala-php", 14};
    DMITIGR_ASSERT(na4.name() == "ala-php");
    DMITIGR_ASSERT(na4.data());
    DMITIGR_ASSERT(pgfe::to<int>(*na4.data()) == 14);
  }

  // Connect.
  const auto conn = pgfe::test::make_connection();
  conn->connect();
  DMITIGR_ASSERT(conn->is_connected());

  // Online test 1 (lvalue named argument).
  {
    const a id{"id", 12345};
    conn->execute([](auto&& row)
    {
      DMITIGR_ASSERT(pgfe::to<int>(row[0]) == 12345);
    }, "select :id", id);
  }

  // Online test 2 (rvalue named argument).
  conn->execute([](auto&& row)
  {
    DMITIGR_ASSERT(pgfe::to<int>(row[0]) == 12345);
  }, "select :id", a{"id", 12345});
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
