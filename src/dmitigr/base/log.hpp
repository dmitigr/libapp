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

#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace dmitigr::log {

/// A log level.
enum class Level {
  emergency = 0,
  alert = 1,
  critical = 2,
  error = 3,
  warning = 4,
  notice = 5,
  info = 6,
  debug = 7
};

/// @returns The text representation of `value`.
inline const char* to_literal(const Level value) noexcept
{
  using enum Level;
  switch (value) {
  case emergency: return "emergency";
  case alert: return "alert";
  case critical: return "critical";
  case error: return "error";
  case warning: return "warning";
  case notice: return "notice";
  case info: return "info";
  case debug: return "debug";
  }
  return nullptr;
}

/// @returns The text representation of `value`.
inline std::string_view to_string_view(const Level level)
{
  if (const auto* const result = to_literal(level))
    return result;
  else
    throw std::invalid_argument{"cannot convert dmitigr::log::Level to text"};
}

/// @returns The binary representation of `value`.
inline Level to_level(const std::string_view value)
{
  using enum Level;
  if (value == "emergency")
    return emergency;
  else if (value == "alert")
    return alert;
  else if (value == "critical")
    return critical;
  else if (value == "error")
    return error;
  else if (value == "warning")
    return warning;
  else if (value == "notice")
    return notice;
  else if (value == "info")
    return info;
  else if (value == "debug")
    return debug;
  throw std::invalid_argument{"cannot convert text to dmitigr::log::Level"};
}

/// A log level value.
inline std::atomic<Level> level{Level::error};

namespace detail {

inline std::ofstream stderr_file_stream;

/**
 * @brief Writes `args` to the `os` according to format string `fmt`.
 *
 * @details If `is_with_now` then writes the result of dmitigr::chrono::now() to
 * the `os` before writing `args`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
inline void write(std::ostream& os, const Level level,
  const std::string_view fmt, std::format_args&& args)
{
  if (level <= log::level) {
    static std::mutex mutex;
    const std::lock_guard lg{mutex};
#ifdef __linux__
    os<<"<"<<static_cast<int>(level)<<">";
#else
    (void)level;
#endif
#ifdef DMITIGR_LOG_WITH_NOW
    os << dmitigr::chrono::now() << ": ";
#endif
    os << std::vformat(fmt, args);
  }
}

} // namespace detail

/**
 * @brief Redirects the both stantard error streams (`std::cerr` and
 * `std::clog`) to the file at `path`.
 */
inline void redirect(const std::filesystem::path& path,
  const std::ios_base::openmode openmode = std::ios_base::app | std::ios_base::ate)
{
  if ( (detail::stderr_file_stream = std::ofstream{path, openmode})) {
    std::cerr.rdbuf(detail::stderr_file_stream.rdbuf());
    std::clog.rdbuf(detail::stderr_file_stream.rdbuf());
  } else
    throw Exception{"cannot open log file " + path.string()};
}

/**
 * @brief Writes `args` to the `std::cerr`.
 *
 * @details If `is_with_now` then writes the result of dmitigr::chrono::now() to
 * the `std::cerr` before writing `args`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void cerr(const Level level, std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, level, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Writes `args` to the `std::cerr` with `Level::error`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void cerr(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, Level::error, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Writes `args` to the `std::clog`.
 *
 * @details If `is_with_now` then writes the result of dmitigr::chrono::now() to
 * the `std::clog` before writing `args`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void clog(const Level level, std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, level, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Writes `args` to the `std::clog` with `Level::error`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void clog(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, Level::error, fmt.get(), std::make_format_args(args...));
}

} // namespace dmitigr::log

#endif  // DMITIGR_BASE_LOG_HPP
