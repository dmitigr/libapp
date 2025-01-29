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

#include "../assert.hpp"
#include "../fifo.hpp"

#include <string_view>

int main()
{
  try {
    using dmitigr::Fifo_array;
    {
      Fifo_array<char, 0> a;
      DMITIGR_ASSERT(a.empty());
    }

    {
      Fifo_array<char, 7> a;
      DMITIGR_ASSERT(a.size() == 0);
      a.push_back('d');
      a.push_back('m');
      a.push_back('i');
      a.push_back('t');
      a.push_back('i');
      a.push_back('g');
      a.push_back('r');
      DMITIGR_ASSERT(a.size() == 7);
      DMITIGR_ASSERT((std::string_view{a.data(), a.size()} == std::string_view{"dmitigr"}));
    }

    {
      Fifo_array<char, 128> a;
      DMITIGR_ASSERT(a.size() == 0);

      a.push_back('D');
      DMITIGR_ASSERT(a.size() == 1);
      DMITIGR_ASSERT(a.front() == 'D');
      DMITIGR_ASSERT(a.back() == 'D');

      a.push_back('I');
      DMITIGR_ASSERT(a.size() == 2);
      DMITIGR_ASSERT(a.front() == 'D');
      DMITIGR_ASSERT(a.back() == 'I');

      a.pop_front();
      DMITIGR_ASSERT(a.size() == 1);
      DMITIGR_ASSERT(a.front() == 'I');
      DMITIGR_ASSERT(a.back() == 'I');

      a.pop_front();
      DMITIGR_ASSERT(a.size() == 0);

      a.unpop_front();
      DMITIGR_ASSERT(a.size() == 1);
      DMITIGR_ASSERT(a.front() == 'I');
      DMITIGR_ASSERT(a.back() == 'I');

      for (unsigned i = 0; i < 10; ++i) a.unpop_front();
      DMITIGR_ASSERT(a.size() == 2);
      DMITIGR_ASSERT(a.front() == 'D');
      DMITIGR_ASSERT(a.back() == 'I');

      a.pop_front();
      a.pop_front();
      DMITIGR_ASSERT(a.empty());
      for (unsigned i = 0; i < 10; ++i) a.unpop_all();
      DMITIGR_ASSERT(a.size() == 2);
      DMITIGR_ASSERT(a.front() == 'D');
      DMITIGR_ASSERT(a.back() == 'I');
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
