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
#include "../base/thread.hpp"
#include "../nix/cpu.hpp"

#include <boost/asio.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>
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

  /**
   * @brief Constucts the shard and pins threads according to `io_pin_index`
   * and `worker_pinmap`.
   */
  Shard(const unsigned int io_pin_index,
    const std::size_t worker_count,
    const std::vector<unsigned int>& worker_pinmap)
    : io_guard_{boost::asio::make_work_guard(io_ctx_)}
  {
    DMITIGR_CKARG(worker_count > 0);
    DMITIGR_CKARG(worker_pinmap.size() > 0);
    io_ = std::thread{&boost::asio::io_context::run, &io_ctx_};
    dmitigr::thread::set_affinity(io_, io_pin_index);
    workers_ = std::make_unique<dmitigr::thread::Pool>(worker_count, worker_pinmap);
  }

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
 * @brief Makes `count` CPU shards with thread pinning.
 *
 * @todo Make shards without thread pinning on non-Linux.
 */
inline std::vector<std::unique_ptr<Shard>>
make_shards(unsigned int count,
  const unsigned int core_offset,
  const std::size_t worker_thread_count,
  std::size_t worker_core_count)
{
  DMITIGR_CKARG(worker_thread_count > 0);

  // Get SMT availability.
  const auto is_smt_avail = nix::is_smt_available();

  // Get CPUs.
  const auto cpus = [core_offset]
  {
    std::vector<nix::Cpu> cpus;
    nix::for_each_cpu([&cpus](auto&& cpu)
    {
      if (cpu.is_physical() && cpu.is_performant())
        cpus.push_back(std::move(cpu));
      return true;
    }, core_offset);
    return cpus;
  }();

  if (!worker_core_count)
    worker_core_count = !is_smt_avail ? std::min<unsigned int>(7, cpus.size()) : 1;

  if (!count)
    count = cpus.size() / (worker_core_count + !is_smt_avail);

  if (cpus.size() < count * (worker_core_count + !is_smt_avail))
    throw std::runtime_error{"insufficient of CPU cores to make "+
      std::to_string(count)+" CPU shards"};

  std::vector<std::unique_ptr<Shard>> result(count);
  for (unsigned int shard_i{}; shard_i < count; ++shard_i) {
    const auto& first_cpu = cpus[shard_i * worker_core_count];

    const unsigned int io_pin_index{is_smt_avail ?
      first_cpu.core_list().at(1).lower() : first_cpu.index()};

    const auto worker_pinmap = [&]
    {
      std::vector<unsigned int> worker_pinmap;
      worker_pinmap.reserve(worker_core_count - !is_smt_avail);
      for (unsigned int i{!is_smt_avail}; i < worker_core_count; ++i)
        worker_pinmap.push_back(cpus[shard_i * worker_core_count + i].index());
      return worker_pinmap;
    }();

    result[shard_i] = std::make_unique<Shard>(io_pin_index,
      worker_thread_count, worker_pinmap);
  }

  return result;
}

} // namespace dmitigr::io

#endif  // DMITIGR_IO_SHARD_HPP
