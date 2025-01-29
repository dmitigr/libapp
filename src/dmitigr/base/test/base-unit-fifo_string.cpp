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

int main()
{
  try {
    using dmitigr::Fifo_string;

    {
      Fifo_string s;
      DMITIGR_ASSERT(s.empty());
    }

    {
      Fifo_string s{"dmitigr"};
      DMITIGR_ASSERT(s.size() == 7);
      DMITIGR_ASSERT(s.view() == "dmitigr");
    }

    {
      Fifo_string s{"dmitigr", 5};
      DMITIGR_ASSERT(s.size() == 5);
      DMITIGR_ASSERT(s.view() == "dmiti");
    }

    {
      Fifo_string s(5, 'd');
      DMITIGR_ASSERT(s.size() == 5);
      DMITIGR_ASSERT(s.view() == "ddddd");
    }

    {
      Fifo_string s;
      DMITIGR_ASSERT(s.size() == 0);

      s.push_back('D');
      DMITIGR_ASSERT(s.size() == 1);
      DMITIGR_ASSERT(s.view() == "D");
      DMITIGR_ASSERT(s.front() == 'D');
      DMITIGR_ASSERT(s.back() == 'D');

      s.push_back('I');
      DMITIGR_ASSERT(s.size() == 2);
      DMITIGR_ASSERT(s.view() == "DI");
      DMITIGR_ASSERT(s.front() == 'D');
      DMITIGR_ASSERT(s.back() == 'I');

      s.pop_front();
      DMITIGR_ASSERT(s.size() == 1);
      DMITIGR_ASSERT(s.view() == "I");
      DMITIGR_ASSERT(s.front() == 'I');
      DMITIGR_ASSERT(s.back() == 'I');

      s.pop_front();
      DMITIGR_ASSERT(s.size() == 0);
      DMITIGR_ASSERT(s.view().empty());

      s.unpop_front();
      DMITIGR_ASSERT(s.size() == 1);
      DMITIGR_ASSERT(s.view() == "I");
      DMITIGR_ASSERT(s.front() == 'I');
      DMITIGR_ASSERT(s.back() == 'I');

      for (unsigned i = 0; i < 10; ++i) s.unpop_front();
      DMITIGR_ASSERT(s.size() == 2);
      DMITIGR_ASSERT(s.view() == "DI");
      DMITIGR_ASSERT(s.front() == 'D');
      DMITIGR_ASSERT(s.back() == 'I');

      s.pop_front();
      s.pop_front();
      DMITIGR_ASSERT(s.empty());
      for (unsigned i = 0; i < 10; ++i) s.unpop_all();
      DMITIGR_ASSERT(s.size() == 2);
      DMITIGR_ASSERT(s.view() == "DI");
      DMITIGR_ASSERT(s.front() == 'D');
      DMITIGR_ASSERT(s.back() == 'I');
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
