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

#include "../assert.hpp"
#include "../buffer.hpp"

#define ASSERT DMITIGR_ASSERT

struct Buffer_data final {
  int* data;
  std::size_t size;
  std::size_t capacity;
};

int main()
{
  try {
    using Buffer = dmitigr::Basic_buffer<Buffer_data>;

    {
      Buffer buf;
      ASSERT(!buf);
    }

    {
      Buffer buf{7};
      ASSERT(buf);
      ASSERT(buf.size() == 0);
      ASSERT(buf.capacity() == 7);
      buf.destructive_resize(buf.capacity());
      ASSERT(buf.size() == buf.capacity());
      for (std::size_t i{}; i < buf.size(); ++i)
        buf.data()[i] = i;

      const auto prev_size = buf.size();
      buf.resize(100);
      ASSERT(buf.size() == 100);
      ASSERT(buf.capacity() == 100);
      for (std::size_t i{prev_size}; i < buf.size(); ++i)
        buf.data()[i] = i;
      ASSERT(buf.size() == buf.capacity());

      for (std::size_t i{}; i < prev_size; ++i)
        ASSERT(buf.data()[i] == static_cast<int>(i));
    }

    {
      Buffer buf;
      ASSERT(!buf);
      buf.destructive_reserve(7);
      ASSERT(buf);
      ASSERT(buf.size() == 0);
      ASSERT(buf.capacity() == 7);
      buf.destructive_resize(10);
      ASSERT(buf.size() == 10);
      ASSERT(buf.capacity() == 10);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
