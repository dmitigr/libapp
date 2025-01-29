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

#ifndef DMITIGR_BASE_THREAD_HPP
#define DMITIGR_BASE_THREAD_HPP

#include "assert.hpp"
#include "exceptions.hpp"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#endif  // __linux__

namespace dmitigr::thread {

// -----------------------------------------------------------------------------
// Pool
// -----------------------------------------------------------------------------

/// Simple thread pool.
class Pool final {
public:
  /// A task.
  using Task = std::function<void()>;

  /// A logger.
  using Logger = std::function<void(std::string_view)>;

  /// The destructor.
  ~Pool()
  {
    // Stop queue.
    {
      const std::lock_guard lg{queue_.mutex};
      queue_.is_started = false;
    }
    queue_.changed.notify_all();

    // Join pool.
    {
      const std::lock_guard lg{pool_.mutex};
      for (auto& thread : pool_.threads) {
        DMITIGR_ASSERT(thread.joinable());
        thread.join();
      }
    }
  }

  /// @name Constructors
  /// @{

  /// Non copy-consructible.
  Pool(const Pool&) = delete;

  /// Non copy-assignable.
  Pool& operator=(const Pool&) = delete;

  /// Non move-constructible.
  Pool(Pool&&) = delete;

  /// Non move-assignable.
  Pool& operator=(Pool&&) = delete;

  /**
   * @brief Constructs the thread pool of size `std::thread::hardware_concurrency()`.
   *
   * @see Pool(std::size_t, Logger)
   */
  Pool(Logger logger = {})
    : Pool{std::thread::hardware_concurrency(), std::move(logger)}
  {}

  /**
   * @brief Constructs the thread pool of the given `size`.
   *
   * @param size The size of the thread pool.
   * @param logger A logger to use to report a error message of an exception
   * thrown in a thread.
   *
   * @par Requires
   * `size > 0`.
   */
  explicit Pool(const std::size_t size, Logger logger = {})
    : logger_{std::move(logger)}
  {
    if (!size)
      throw Exception{"cannot create thread pool: empty pool is not allowed"};

    queue_.is_started = true;
    pool_.threads.reserve(size);
    for (std::size_t i{}; i < size; ++i)
      pool_.threads.emplace_back(&Pool::wait_and_run, this);
  }

  /// @}

  /**
   * @brief Submit the task to run on the thread pool.
   *
   * @par Requires
   * `task`.
   */
  void submit(Task task)
  {
    if (!task)
      throw Exception{"cannot submit invalid task to thread pool"};

    const std::lock_guard lg{queue_.mutex};
    queue_.tasks.push(std::move(task));
    queue_.changed.notify_one();
  }

  /// Clears the queue of unstarted tasks.
  void clear() noexcept
  {
    const std::lock_guard lg{queue_.mutex};
    queue_.tasks = {};
  }

  /// @returns The size of task queue.
  std::size_t queue_size() const noexcept
  {
    const std::lock_guard lg{queue_.mutex};
    return queue_.tasks.size();
  }

  /// @returns The thread pool size.
  std::size_t size() const noexcept
  {
    const std::lock_guard lg{pool_.mutex};
    return pool_.threads.size();
  }

private:
  struct {
    mutable std::mutex mutex;
    std::condition_variable changed;
    std::queue<Task> tasks;
    bool is_started{};
  } queue_;

  struct {
    mutable std::mutex mutex;
    std::vector<std::thread> threads;
  } pool_;

  Logger logger_;

  void wait_and_run() noexcept
  {
    while (true) {
      try {
        Task task;
        {
          std::unique_lock lk{queue_.mutex};
          queue_.changed.wait(lk, [this]
          {
            return !queue_.tasks.empty() || !queue_.is_started;
          });
          if (queue_.is_started) {
            DMITIGR_ASSERT(!queue_.tasks.empty());
            task = std::move(queue_.tasks.front());
            DMITIGR_ASSERT(task);
            queue_.tasks.pop();
          } else
            return;
        }
        task();
      } catch (const std::exception& e) {
        log_error(e.what());
      } catch (...) {
        log_error("unknown error");
      }
    }
  }

  void log_error(const std::string_view what) const noexcept
  {
    try {
      if (logger_)
        logger_(what);
    } catch (...){}
  }
};

// -----------------------------------------------------------------------------
// Affinity
// -----------------------------------------------------------------------------

#ifdef __linux__
/// Sets the CPU affinity of the thread `handle` to the `cpu`.
inline std::error_code set_affinity(const pthread_t handle,
  const unsigned int cpu) noexcept
{
  if (!(handle && cpu < std::thread::hardware_concurrency()))
    return std::make_error_code(std::errc::invalid_argument);

  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  const int err{pthread_setaffinity_np(handle, sizeof(cpu_set_t), &set)};
  return std::error_code{err, std::generic_category()};
}
#endif

/// @overload
inline std::error_code set_affinity(std::thread& thread,
  const unsigned int cpu) noexcept
{
#ifdef __linux__
  return set_affinity(thread.native_handle(), cpu);
#else
  (void)thread;
  (void)cpu;
  return std::errc::not_supported;
#endif
}

} // namespace dmitigr::thread

#endif  // DMITIGR_BASE_THREAD_HPP
