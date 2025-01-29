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

#ifndef DMITIGR_BASE_LOG_HPP
#define DMITIGR_BASE_LOG_HPP

#include "chrono.hpp"
#include "exceptions.hpp"
#include "filesystem.hpp"

#include <atomic>
#include <fstream>
#include <iostream>

namespace dmitigr::log {

namespace detail {
inline std::ofstream log_file_stream;
} // namespace detail

/// @see clog().
inline std::atomic_bool is_clog_with_now;

/// Redirecting `std::clog` to file at `path`.
inline void redirect_clog(const std::filesystem::path& path,
  const std::ios_base::openmode openmode = std::ios_base::app | std::ios_base::ate)
{
  if ( (detail::log_file_stream = std::ofstream{path, openmode}))
    std::clog.rdbuf(detail::log_file_stream.rdbuf());
  else
    throw Exception{"cannot open log file " + path.string()};
}

/// @returns std::clog after writing now string to it first.
inline std::ostream& clog_now() noexcept
{
  return std::clog << dmitigr::chrono::now() << ": ";
}

/**
 * @returns The stream for starting log entry.
 *
 * @par Effects
 * If `is_clog_with_now` then puts the result of clog_now() to the `std::clog`
 * before return.
 */
inline std::ostream& clog() noexcept
{
  return is_clog_with_now ? clog_now() : std::clog;
}

} // namespace dmitigr::log

#endif  // DMITIGR_BASE_LOG_HPP
