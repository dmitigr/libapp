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

#include "../base/thread.hpp"

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
    if (!(worker_count > 0))
      throw std::invalid_argument{"dmitigr::io::Shard: invalid worker_count"};
    else if (!(worker_pinmap.size() > 0))
      throw std::invalid_argument{"dmitigr::io::Shard: invalid worker_pinmap"};

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
  boost::asio::io_context io_ctx_;
  boost::asio::executor_work_guard<decltype(io_ctx_)::executor_type> io_guard_;
  std::thread io_;
  std::unique_ptr<dmitigr::thread::Pool> workers_;
};

} // namespace dmitigr::io

#endif  // DMITIGR_IO_SHARD_HPP
