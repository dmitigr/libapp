// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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
#include "../cf.hpp"

#define ASSERT DMITIGR_ASSERT

int main()
{
  using std::cout;
  using std::cerr;
  using std::endl;
  try {
    namespace mac = dmitigr::mac;

    using mac::cf::Is_handle_v;
    static_assert(Is_handle_v<mac::cf::Bundle>);
    static_assert(!Is_handle_v<int>);

    {
      const int num_in{1983};
      const auto num = mac::cf::number::create(num_in);
      const auto num_out = mac::cf::number::to<int>(num);
      ASSERT(num_in == num_out);
    }

    {
      const float num_in{1983.0406};
      const auto num = mac::cf::number::create(num_in);
      const auto num_out = mac::cf::number::to<float>(num);
      ASSERT(num_in == num_out);
    }

    {
      const double num_in{1983.0406};
      const auto num = mac::cf::number::create(num_in);
      const auto num_out = mac::cf::number::to<double>(num);
      ASSERT(num_in == num_out);
    }

    {
      const char* const str_in{""};
      const auto str = mac::cf::string::create_no_copy(str_in);
      const auto str_out = mac::cf::string::to_string(str);
      ASSERT(str_in == str_out);
    }

    {
      const char* const str_in{"Dima"};
      const auto str = mac::cf::string::create_no_copy(str_in);
      const auto str_out = to_string(str); // ADL test
      ASSERT(str_in == str_out);
    }

    {
      mac::cf::Dictionary dict;
      try {
        (void)mac::cf::dictionary::value<int>(dict, "key");
      } catch (...) {}
    }
  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  } catch (...) {
    cerr << "unknown error" << endl;
    return 2;
  }
}
