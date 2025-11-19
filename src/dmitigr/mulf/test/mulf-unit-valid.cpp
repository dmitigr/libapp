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
#include "../../base/stream.hpp"
#include "../../mulf/mulf.hpp"

#include <filesystem>

int main(int, char* argv[])
{
  try {
    namespace mulf = dmitigr::mulf;
    using mulf::Form_data;

    const std::filesystem::path this_exe_file_name{argv[0]};
    const auto this_exe_dir_name = this_exe_file_name.parent_path();
    const auto form_data = dmitigr::read_to_string(this_exe_dir_name /
      "mulf-form-data-valid1.txt");

    const std::string boundary{"AaB03x"};
    const Form_data data{form_data, boundary};
    DMITIGR_ASSERT(data.entry_count() == 2);

    {
      const auto& e = data.entry(0);
      DMITIGR_ASSERT(e.name() == "field1");
      DMITIGR_ASSERT(!e.filename());
      DMITIGR_ASSERT(e.content_type() == "text/plain");
      DMITIGR_ASSERT(e.charset() == "UTF-8");
      DMITIGR_ASSERT(e.content() == "Field1 data.");
    }

    {
      const auto& e = data.entry(1);
      DMITIGR_ASSERT(e.name() == "field2");
      DMITIGR_ASSERT(e.filename() == "text.txt");
      DMITIGR_ASSERT(e.content_type() == "text/plain");
      DMITIGR_ASSERT(e.charset() == "utf-8");
      DMITIGR_ASSERT(e.content() == "Field2 data.");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
