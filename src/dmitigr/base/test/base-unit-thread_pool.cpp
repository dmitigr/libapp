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
#include "../../base/thread.hpp"

#include <chrono>
#include <thread>

int main()
{
  namespace thread = dmitigr::thread;

  try {
    const auto size = std::thread::hardware_concurrency() * 2;
    thread::Pool pool{size};
    DMITIGR_ASSERT(pool.size() == size);
    DMITIGR_ASSERT(pool.queue_size() == 0);

    for (std::size_t i = 0; i < 16*size; ++i) {
      pool.submit([]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
        std::cout << "Hello from thread " << std::this_thread::get_id() << std::endl;
      });
    }

    while (pool.queue_size()) {
      std::cout << "Thread pool has " << pool.queue_size() << " uncompleted tasks" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds{8});
    }
    DMITIGR_ASSERT(pool.queue_size() == 0);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
