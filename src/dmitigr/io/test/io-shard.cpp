// -*- C++ -*-
//
// Copyright 2026 Dmitry Igrishin
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
#include "../shard.hpp"

#include <iostream>

#define ASSERT DMITIGR_ASSERT

int main()
{
  using std::cout;
  using std::endl;
  namespace io = dmitigr::io;

  {
    const auto shards = io::make_shared_shards({
      .count = 0,
      .worker_thread_count = 3});
#ifndef __linux__
    ASSERT(shards.size() == 1);
#endif
    ASSERT(shards.front()->workers().size() == 3);
    cout << "Shared shards:" << endl
         << "  count: " << ssize(shards) << endl
         << "  worker thread count: " << shards.front()->workers().size()
         << endl;
  }

  {
    const auto shards = io::make_shared_shards({
      .count = 2,
      .worker_thread_count = 4});
#ifndef __linux__
    ASSERT(shards.size() == 2);
#endif
    ASSERT(shards.front()->workers().size() == 4);
    cout << "Shared shards:" << endl
         << "  count: " << ssize(shards) << endl
         << "  worker thread count: " << shards.front()->workers().size()
         << endl;
  }

#ifdef __linux__
  {
    const auto shards = io::make_shared_shards({
      .count = 2,
      .worker_thread_count = 4,
      .worker_core_count = 2
      });
#ifndef __linux__
    ASSERT(shards.size() == 2);
#endif
    ASSERT(shards.front()->workers().size() == 4);
    cout << "Shared shards:" << endl
         << "  count: " << ssize(shards) << endl
         << "  worker thread count: " << shards.front()->workers().size()
         << endl;
  }
#endif
}
