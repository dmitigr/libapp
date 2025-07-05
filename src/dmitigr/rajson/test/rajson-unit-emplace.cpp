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
#include "../../rajson/rajson.hpp"

int main()
{
  try {
    namespace rajson = dmitigr::rajson;
    rapidjson::Document doc{rapidjson::kObjectType};
    auto& alloc = doc.GetAllocator();
    const rajson::Path bar_baz{"bar/baz"};
    const rajson::Path foo_bar_baz{"foo/bar/baz"};
    const rajson::Path foo_bar{"foo/bar"};
    auto err = rajson::emplace(doc, alloc, foo_bar_baz, rapidjson::Value{"Dima"});
    DMITIGR_ASSERT(!err);
    err = rajson::emplace(doc, alloc, bar_baz, 39);
    DMITIGR_ASSERT(!err);
    err = rajson::emplace(doc, alloc, foo_bar_baz, std::string{"Olga"});
    DMITIGR_ASSERT(!err);
    err = rajson::emplace(doc, alloc, foo_bar, rapidjson::Value{"Vika"});
    DMITIGR_ASSERT(!err);
    std::cout << rajson::to_text(doc) << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
