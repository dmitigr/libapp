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
    using rajson::to;
    using rajson::to_value;
    rapidjson::Document doc{rapidjson::kObjectType};
    auto& alloc = doc.GetAllocator();

    {
      std::vector<int> vi{1,2,3};
      auto val = to_value(vi, alloc);
      auto vi_copy = to<std::vector<int>>(val);
      DMITIGR_ASSERT(vi == vi_copy);
    }

    {
      std::vector<std::optional<int>> vi{1,std::nullopt,3};
      auto val = to_value(vi, alloc);
      auto vi_copy = to<std::vector<std::optional<int>>>(val);
      DMITIGR_ASSERT(vi == vi_copy);
    }

    {
      std::vector<float> vf{1.0f,2.0f,3.0f};
      auto val = to_value(vf, alloc);
      auto vf_copy = to<std::vector<float>>(val);
      DMITIGR_ASSERT(vf == vf_copy);
    }

    {
      std::vector<std::optional<float>> vf{1.0f,std::nullopt,3.0f};
      auto val = to_value(vf, alloc);
      auto vf_copy = to<std::vector<std::optional<float>>>(val);
      DMITIGR_ASSERT(vf == vf_copy);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
