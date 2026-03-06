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

#ifndef DMITIGR_IO_SHARD_HPP
#define DMITIGR_IO_SHARD_HPP

#include "../base/assert.hpp"
#include "../base/concepts.hpp"
#include "../base/thread.hpp"
#ifdef __linux__
#include "../nix/cpu.hpp"
#endif

#include <boost/asio.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::io {

/// A CPU shard with 1 IO thread and multiple worker threads.
class Shard final {
public:
  /// Joins threads.
  ~Shard()
  {
    try {
      io_guard_.reset();
      if (io_.joinable())
        io_.join();
    } catch (...) {}
  }

  /// Constucts the shard without thread pinning.
  explicit Shard(const int worker_count)
    : io_guard_{boost::asio::make_work_guard(io_ctx_)}
  {
    DMITIGR_CKARG(worker_count > 0);
    io_ = std::thread{&boost::asio::io_context::run, &io_ctx_};
    workers_ = std::make_unique<dmitigr::thread::Pool>(worker_count);
  }

#ifdef __linux__
  /**
   * @brief Constucts the shard and pins threads according to `io_pin_index`
   * and `worker_pinmap`.
   */
  Shard(const int io_pin_index,
    const int worker_count,
    const std::vector<int>& worker_pinmap)
    : io_guard_{boost::asio::make_work_guard(io_ctx_)}
  {
    DMITIGR_CKARG(worker_count > 0);
    DMITIGR_CKARG(std::ssize(worker_pinmap) > 0);
    io_ = std::thread{&boost::asio::io_context::run, &io_ctx_};
    dmitigr::thread::set_affinity(io_, io_pin_index);
    workers_ = std::make_unique<dmitigr::thread::Pool>(worker_count, worker_pinmap);
  }
#endif

  /// @returns IO-context.
  boost::asio::io_context& io_context() noexcept
  {
    return io_ctx_;
  }

  /// @overload
  const boost::asio::io_context& io_context() const noexcept
  {
    return io_ctx_;
  }

  /// @returns Workers.
  dmitigr::thread::Pool& workers() noexcept
  {
    return *workers_;
  }

  /// @overload
  const dmitigr::thread::Pool& workers() const noexcept
  {
    return *workers_;
  }

private:
  boost::asio::io_context io_ctx_{1};
  boost::asio::executor_work_guard<decltype(io_ctx_)::executor_type> io_guard_;
  std::thread io_;
  std::unique_ptr<dmitigr::thread::Pool> workers_;
};

/**
 * @brief Arguments for make_shards(const Make_shards_args&).
 */
struct Make_shards_args final {
  /**
   * @brief A number of shards to make.
   *
   * @remarks Must not be negative. `0` denotes "calculate automatically".
   */
  int count{};

  /**
   * @brief A number of worker threads per shard.
   *
   * @remarks Must be positive.
   */
  int worker_thread_count{1};


  /**
   * @brief A number of cores dedicated to worker threads.
   *
   * @remarks Must not be negative. Must be `0` on non-Linux.
   */
  int worker_core_count{};

  /**
   * @brief An index of the core starting from which to select cores for pinning.
   *
   * @remarks Must not be negative. Must be `0` on non-Linux.
   */
  int core_offset{
#ifdef __linux__
    1
#else
    0
#endif
  };

  /**
   * @brief The flag indicating the need to consider of using SMT.
   *
   * @remarks Must be `false` on non-Linux.
   */
  bool is_consider_smt{
#ifdef __linux__
    true
#else
    false
#endif
  };

  /**
   * @brief The flag indicating the need of pinning threads to CPU cores.
   *
   * @remarks Must be `false` on non-Linux.
   */
  bool is_thread_pinning{
#ifdef __linux__
    true
#else
    false
#endif
  };
};

/// Makes CPU shards.
template<Pure_type SmartPtr>
std::vector<SmartPtr> make_shards(const Make_shards_args& args = {})
{
  DMITIGR_CKARG(args.count >= 0);
  DMITIGR_CKARG(args.worker_thread_count > 0);
  DMITIGR_CKARG(args.worker_core_count >= 0);
  DMITIGR_CKARG(args.core_offset >= 0);
  DMITIGR_CKARG(!args.is_consider_smt || args.is_thread_pinning);
  DMITIGR_CKARG(args.is_thread_pinning || args.worker_core_count == 0);
#ifndef __linux
  DMITIGR_CKARG(args.worker_core_count == 0);
  DMITIGR_CKARG(args.core_offset == 0);
  DMITIGR_CKARG(!args.is_consider_smt);
  DMITIGR_CKARG(!args.is_thread_pinning);
#endif

  // Get SMT availability.
#ifdef __linux__
  const auto is_with_smt = args.is_consider_smt &&
    nix::smt_status() == nix::Smt_status::on;
#else
  const auto is_with_smt = false;
#endif

  // Get CPUs.
#ifdef __linux__
  const auto cpus = args.is_thread_pinning ? [&]
  {
    std::vector<nix::Cpu> cpus;
    nix::for_each_cpu([&](auto&& cpu)
    {
      if (cpu.is_online() && cpu.is_physical() && cpu.is_performant() &&
        (!is_with_smt || cpu.is_smt_available()))
        cpus.push_back(std::move(cpu));
      return true;
    }, args.core_offset);
    return cpus;
  }() : std::vector<nix::Cpu>{};
  const auto cpu_count = ssize(cpus);
#else
  const std::ptrdiff_t cpu_count{};
#endif

  const auto worker_core_count = [&]() -> std::ptrdiff_t
  {
    if (args.worker_core_count == 0)
      return !is_with_smt ? std::min<std::ptrdiff_t>(7, cpu_count) : 1;
    return args.worker_core_count;
  }();

  const auto total_core_count = bool(worker_core_count) *
    (worker_core_count + !is_with_smt);

  const auto count = [&]() -> std::ptrdiff_t
  {
    if (args.count == 0)
      return cpu_count > 0 ? (cpu_count / total_core_count) : 1;
    else
      return args.count;
  }();

  if (cpu_count < count * total_core_count)
    throw std::runtime_error{"dmitigr::io: insufficient of "
      "CPU cores to make "+std::to_string(count)+" CPU shards"};

  std::vector<SmartPtr> result;
  result.reserve(count);
  if (args.is_thread_pinning) {
#ifdef __linux__
    for (int shard_i{}; shard_i < count; ++shard_i) {
      const auto shard_offset = shard_i * total_core_count;
      const auto& first_cpu = cpus[shard_offset];

      const auto io_pin_index = [&]
      {
        if (is_with_smt) {
          if (const auto core_list = first_cpu.core_list(); core_list.size() > 1)
            return core_list[1].lower();
          else
            throw std::runtime_error{"dmitigr::io: cannot get core index for IO "
              "pinning: CPU configuration changed"};
        }
        return first_cpu.index();
      }();

      const auto worker_pinmap = [&]
      {
        std::vector<int> pinmap;
        pinmap.reserve(worker_core_count);
        for (int i{}; i < worker_core_count; ++i)
          pinmap.push_back(cpus[shard_offset + i + !is_with_smt].index());
        return pinmap;
      }();

      if constexpr (std::is_same_v<SmartPtr, std::unique_ptr<Shard>>)
        result.push_back(std::make_unique<Shard>(io_pin_index,
          args.worker_thread_count, worker_pinmap));
      else if constexpr (std::is_same_v<SmartPtr, std::shared_ptr<Shard>>)
        result.push_back(std::make_shared<Shard>(io_pin_index,
          args.worker_thread_count, worker_pinmap));
      else
        result.emplace_back(new Shard{io_pin_index,
          args.worker_thread_count, worker_pinmap});
    }
#else
    DMITIGR_ASSERT(false);
#endif
  } else {
    for (int shard_i{}; shard_i < count; ++shard_i) {
      if constexpr (std::is_same_v<SmartPtr, std::unique_ptr<Shard>>)
        result.push_back(std::make_unique<Shard>(args.worker_thread_count));
      else if constexpr (std::is_same_v<SmartPtr, std::shared_ptr<Shard>>)
        result.push_back(std::make_shared<Shard>(args.worker_thread_count));
      else
        result.emplace_back(new Shard{args.worker_thread_count});
    }
  }
  return result;
}

/// Forwards call to make_shards<std::shared_ptr<Shard>>(args).
inline auto make_shared_shards(const Make_shards_args& args = {})
{
  return make_shards<std::shared_ptr<Shard>>(args);
}

/// Forwards call to make_shards<std::unique_ptr<Shard>>(args).
inline auto make_unique_shards(const Make_shards_args& args = {})
{
  return make_shards<std::unique_ptr<Shard>>(args);
}

} // namespace dmitigr::io

#endif  // DMITIGR_IO_SHARD_HPP
