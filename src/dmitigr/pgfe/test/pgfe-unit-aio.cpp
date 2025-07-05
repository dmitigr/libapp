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

/// @NEW: remove me
#define DMITIGR_LIBS_AIO_ASIO

#include "pgfe-unit.hpp"

#include <cstring>

int main()
try {
  namespace pgfe = dmitigr::pgfe;

  {
#ifdef DMITIGR_LIBS_AIO_ASIO
    DMITIGR_LIBS_AIO_ASIO_NAMESPACE::io_context loop;
#else
#error Unknown AIO type
#endif

    auto conn = pgfe::test::make_connection(loop);
    DMITIGR_ASSERT(conn);
    conn->connect_aio([](const auto& err, auto& conn)
    {
      if (err) {
        std::cerr << err.message() << std::endl;
        return;
      }
      DMITIGR_ASSERT(conn.is_connected());
      std::clog << "connected!\n";

      conn.prepare_aio([](const auto& err, auto& conn)
      {
        if (err) {
          std::cerr << err.message() << std::endl;
          return;
        }
        DMITIGR_ASSERT(!conn.error());
        auto ps = conn.prepared_statement();
        DMITIGR_ASSERT(ps);
        DMITIGR_ASSERT(ps.name() == "ps1");
        std::clog << "prepared!\n";

        conn.describe_aio([](const auto& err, auto& conn)
        {
          if (err) {
            std::cerr << err.message() << std::endl;
            return;
          }
          DMITIGR_ASSERT(!conn.error());
          auto ps = conn.prepared_statement();
          DMITIGR_ASSERT(ps);
          DMITIGR_ASSERT(ps.name() == "ps1");
          std::clog << "described!\n";

          ps.bind(0, "dmitigr").bind(1, 39).execute_aio([](const auto& err, auto& conn)
          {
            if (err) {
              std::cerr << err.message() << std::endl;
              return;
            }
            DMITIGR_ASSERT(!conn.error());
            if (const auto com = conn.completion()) {
              std::clog << "executed!\n";
              return conn.disconnect(); // done
            } else if (const auto row = conn.row()) {
              using pgfe::to;
              DMITIGR_ASSERT(to<std::string_view>(row[0]) == "dmitigr");
              DMITIGR_ASSERT(to<int>(row[1]) == 39);
            }
          });
        }, "ps1");
      }, "select :name, :age", "ps1");
    });

#ifdef DMITIGR_LIBS_AIO_ASIO
    loop.run();
#endif
  }
} catch (const dmitigr::Exception& e) {
  std::cerr << e.err().message() << std::endl;
  return 1;
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
