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
#include "../../rajson/rajson.hpp"
#include "../../str/stream.hpp"

#include <iostream>

struct Db_params final {
  std::string hostname;
  int port;
  std::string database;
};

namespace dmitigr::rajson {
template<> struct Conversions<Db_params> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    Db_params result;
    result.hostname = value.FindMember("hostname")->value.GetString();
    result.port = value.FindMember("port")->value.GetInt();
    result.database = value.FindMember("database")->value.GetString();
    return result;
  }
};
} // namespace dmitigr::rajson

int main(int, char* const argv[])
{
  try {
    namespace rajson = dmitigr::rajson;
    namespace str = dmitigr::str;

    const std::filesystem::path this_exe_file_name{argv[0]};
    const auto this_exe_dir_name = this_exe_file_name.parent_path();
    const auto input = str::read_to_string(this_exe_dir_name /
      "rajson-unit-value_view.json");
    auto document = rajson::document_from_text(input);

    {
      const auto& constant_document = document;
      const rajson::Value_view json{constant_document};
      const auto host = json.mandatory<std::string>("host");
      DMITIGR_ASSERT(host == "localhost");
      const auto port = json.mandatory<int>("port");
      DMITIGR_ASSERT(port == 9001);
      const auto db = json.mandatory<Db_params>("db");
      DMITIGR_ASSERT(db.hostname == "localhost");
      DMITIGR_ASSERT(db.port == 5432);
      DMITIGR_ASSERT(db.database == "postgres");
      //
      {
        const auto dbv = json.mandatory("db");
        const auto hostname = dbv.mandatory<std::string>("hostname");
        const auto portnum = dbv.mandatory<int>("port");
        const auto database = dbv.mandatory<std::string>("database");
        const auto database_v = dbv.mandatory<std::string_view>("database");
        DMITIGR_ASSERT(hostname == "localhost");
        DMITIGR_ASSERT(portnum == 5432);
        DMITIGR_ASSERT(database == "postgres");
        DMITIGR_ASSERT(database_v == "postgres");
      }
    }

    {
      rajson::Value_view json{document};
      json.mandatory("host").value() = "localhost.local";
    }

    std::cout << rajson::to_text(document) << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
