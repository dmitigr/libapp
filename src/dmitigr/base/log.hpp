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

/**
 * @file
 * A simple log library
 */

/**
 * @def DMITIGR_LOG_WITH_LEVEL
 * If defined, the log output is written with log level prefix `<N>`, where
 * `N` is an integer of range `[0, 7]`. The value of dmitigr::log::level defines
 * the maximum level to log.
 */

/**
 * @def DMITIGR_LOG_PREFIX_WRITER
 * Defines the name of a function with signature `void w(std::ostream&, Time)`
 * for writing log entry prefix. The default is DMITIGR_LOG_DEFAULT_PREFIX_WRITER.
 */

/**
 * @def DMITIGR_LOG_NOW
 * Defines the name of a function with signature `Time t()` for getting time
 * point which is passed to DMITIGR_LOG_PREFIX_WRITER. The default is
 * DMITIGR_LOG_DEFAULT_NOW.
 */

/**
 * @def DMITIGR_LOG_WITH_DEFAULT_PREFIX
 * If defined, the default log entry prefix is writen.
 */

/**
 * @def DMITIGR_LOG_CALL_WRITE
 * Defines the routine which called by dmitigr::log::call() to log the error
 * output. The default is DMITIGR_LOG_WRITE().
 */

#ifndef DMITIGR_BASE_LOG_HPP
#define DMITIGR_BASE_LOG_HPP

#include "basics.hpp"
#include "chrono.hpp"
#include "str.hpp"
#include "utility.hpp"

#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#ifdef DMITIGR_LOG_WITH_DEFAULT_PREFIX
#ifndef DMITIGR_LOG_PREFIX_WRITER
#define DMITIGR_LOG_PREFIX_WRITER dmitigr::log::default_prefix_writer
#else
#error DMITIGR_LOG_WITH_DEFAULT_PREFIX conflicts with DMITIGR_LOG_PREFIX_WRITER
#endif
#endif

#if defined(DMITIGR_LOG_PREFIX_WRITER) && !defined(DMITIGR_LOG_NOW)
#define DMITIGR_LOG_NOW std::chrono::system_clock::now
#endif

#ifndef DMITIGR_LOG_CALL_SEVERITY
#define DMITIGR_LOG_CALL_SEVERITY Level::error
#endif

#ifndef DMITIGR_LOG_CALL_ERR_LOG_FMT
#define DMITIGR_LOG_CALL_ERR_LOG_FMT "cannot {}: {}"
#endif

#ifndef DMITIGR_LOG_CALL_WRITE
#define DMITIGR_LOG_CALL_WRITE(...) DMITIGR_LOG_WRITE(__VA_ARGS__)
#endif

/// Expands to call dmitigr::log::emergency().
#define DMITIGR_LOG_EMERGENCY(fmt, ...) do {                    \
    if (dmitigr::log::level >= dmitigr::log::Level::emergency)  \
      dmitigr::log::emergency(fmt __VA_OPT__(,) __VA_ARGS__);   \
  } while (false)

/// Expands to call dmitigr::log::alert().
#define DMITIGR_LOG_ALERT(fmt, ...) do {                    \
    if (dmitigr::log::level >= dmitigr::log::Level::alert)  \
      dmitigr::log::alert(fmt __VA_OPT__(,) __VA_ARGS__);   \
  } while (false)

/// Expands to call dmitigr::log::critical().
#define DMITIGR_LOG_CRITICAL(fmt, ...) do {                     \
    if (dmitigr::log::level >= dmitigr::log::Level::critical)   \
      dmitigr::log::critical(fmt __VA_OPT__(,) __VA_ARGS__);    \
  } while (false)

/// Expands to call dmitigr::log::error().
#define DMITIGR_LOG_ERROR(fmt, ...) do {                    \
    if (dmitigr::log::level >= dmitigr::log::Level::error)  \
      dmitigr::log::error(fmt __VA_OPT__(,) __VA_ARGS__);   \
  } while (false)

/// Expands to call dmitigr::log::warning().
#define DMITIGR_LOG_WARNING(fmt, ...) do {                      \
    if (dmitigr::log::level >= dmitigr::log::Level::warning)    \
      dmitigr::log::warning(fmt __VA_OPT__(,) __VA_ARGS__);     \
  } while (false)

/// Expands to call dmitigr::log::notice().
#define DMITIGR_LOG_NOTICE(fmt, ...) do {                   \
    if (dmitigr::log::level >= dmitigr::log::Level::notice) \
      dmitigr::log::notice(fmt __VA_OPT__(,) __VA_ARGS__);  \
  } while (false)

/// Expands to call dmitigr::log::info().
#define DMITIGR_LOG_INFO(fmt, ...) do {                     \
    if (dmitigr::log::level >= dmitigr::log::Level::info)   \
      dmitigr::log::info(fmt __VA_OPT__(,) __VA_ARGS__);    \
  } while (false)

/// Expands to call dmitigr::log::debug().
#define DMITIGR_LOG_DEBUG(fmt, ...) do {                    \
    if (dmitigr::log::level >= dmitigr::log::Level::debug)  \
      dmitigr::log::debug(fmt __VA_OPT__(,) __VA_ARGS__);   \
  } while (false)

/// Expands to call dmitigr::log::write().
#define DMITIGR_LOG_WRITE(lvl, fmt, ...) do {                   \
    dmitigr::log::write(lvl, fmt __VA_OPT__(,) __VA_ARGS__);    \
  } while (false)

namespace dmitigr::log {

/// Writes a default log prefix to `os`.
inline void default_prefix_writer(std::ostream& os, const auto tp)
{
  const auto time = chrono::to_string_view_iso8601(tp);
  os.put('[');
  os.write(time.data(), time.size());
  os.write("] ", 2);
}

/**
 * @brief A log level.
 *
 * @details
 */
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

/**
 * @brief A maximum level to log.
 *
 * @details Entries with the level greater than specified by this variable is
 * not logged.
 */
inline std::atomic<Level> level{Level::error};

namespace detail {

inline std::ofstream stderr_file_stream;

/**
 * @brief Writes the entry to the `os` according to the format string `fmt`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
inline void write(std::ostream& os, const Level level,
  const std::string_view fmt, std::format_args&& args)
{
  if (level <= log::level) {
#ifdef DMITIGR_LOG_NOW
    const auto now = DMITIGR_LOG_NOW();
#endif
#ifdef DMITIGR_LOG_WITH_LEVEL
    char lvl[3];
    lvl[0] = '<';
    lvl[1] = 48 + static_cast<int>(level);
    lvl[2] = '>';
#else
    (void)level;
#endif

    const auto output = std::vformat(fmt, args);

    static std::mutex mutex;
    const std::lock_guard lg{mutex};
#ifdef DMITIGR_LOG_WITH_LEVEL
    os.write(lvl, sizeof(lvl));
#endif
#ifdef DMITIGR_LOG_PREFIX_WRITER
    DMITIGR_LOG_PREFIX_WRITER(os, now);
#endif
    os.write(output.data(), output.size());
    os.put('\n');
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
    throw std::runtime_error{"cannot redirect stderr to " + path.string()};
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
 * @brief Acts as if called cerr() with Level::emergency.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void emergency(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, Level::emergency, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called cerr() with Level::alert.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void alert(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, Level::alert, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called cerr() with Level::critical.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void critical(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, Level::critical, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called cerr() with Level::error.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void error(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::cerr, Level::error, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called clog() with Level::warning.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void warning(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, Level::warning, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called clog() with Level::notice.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void notice(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, Level::notice, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called clog() with Level::info.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void info(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, Level::info, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Acts as if called clog() with Level::debug.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void debug(std::format_string<Types...> fmt, Types&& ... args)
{
  detail::write(std::clog, Level::debug, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Writes the log entry to the proper stream depending on `level`.
 *
 * @par Thread-safety
 * Thread-safe.
 */
template<typename ... Types>
void write(const Level level, std::format_string<Types...> fmt, Types&& ... args)
{
  static const auto stream = [](const Level level) noexcept -> std::ostream&
  {
    using enum Level;
    switch (level) {
    case emergency:
      [[fallthrough]];
    case alert:
      [[fallthrough]];
    case critical:
      [[fallthrough]];
    case error:
      return std::cerr;
    case warning:
      [[fallthrough]];
    case notice:
      [[fallthrough]];
    case info:
      [[fallthrough]];
    case debug:
      return std::clog;
    }
  };
  detail::write(stream(level), level, fmt.get(), std::make_format_args(args...));
}

/**
 * @brief Calls `callback`, catches exceptions and logs them as errors.
 *
 * @tparam Action An action description.
 * @tparam Severity A severity of action used as log level in case of exception.
 * @tparam ErrorLogFmt A format string of an error message, which is printed
 * in case of exception. The first two placeholders of this string refers to
 * Action and `std::exception::what()` correspondingly.
 * @tparam NoThrow Throw mode switch.
 * @param callback A function to call. It can be either parameter free or accept
 * Action as std::string_view.
 *
 * @returns The result of `callback` if `NoThrow == NoThrow::no`,
 * or Ret<T> otherwise.
 */
template<str::Literal Action,
  Level Severity = DMITIGR_LOG_CALL_SEVERITY,
  str::Literal ErrLogFmt = DMITIGR_LOG_CALL_ERR_LOG_FMT,
  Nothrow NoThrow = Nothrow::no,
  typename F>
requires (std::invocable<F> || std::invocable<F, std::string_view>)
decltype(auto) call(F&& callback) noexcept(NoThrow == Nothrow::yes)
{
  static_assert(str::len(Action) > 0, "action literal required");
  static_assert(Severity <= Level::error, "severity error+ required");
  static_assert(str::len(ErrLogFmt) > 0, "error format literal required");
  constexpr bool can_throw{NoThrow == Nothrow::no};
  try {
    if constexpr (can_throw) {
      if constexpr (std::is_invocable_v<F>)
        return std::forward<F>(callback)();
      else
        return std::forward<F>(callback)(Action.to_string_view());
    } else {
      if constexpr (std::is_invocable_v<F>)
        return dmitigr::call_nothrow(std::forward<F>(callback));
      else
        return dmitigr::call_nothrow(std::forward<F>(callback),
          Action.to_string_view());
    }
  } catch (const std::exception& e) {
    try {
      DMITIGR_LOG_CALL_WRITE(Severity, ErrLogFmt.to_string_view(),
        Action.to_string_view(), e.what());
    } catch (...) {}
    throw;
  } catch (...) {
    try {
      DMITIGR_LOG_CALL_WRITE(Severity, ErrLogFmt.to_string_view(),
        Action.to_string_view(), "unknown error");
    } catch (...) {}
    throw;
  }
}

/// Calls call() with Nothrow::yes.
template<str::Literal Action,
  Level Severity = DMITIGR_LOG_CALL_SEVERITY,
  str::Literal ErrLogFmt = DMITIGR_LOG_CALL_ERR_LOG_FMT,
  typename F>
auto call_nothrow(F&& callback) noexcept
{
  return call<Action, Severity, ErrLogFmt, Nothrow::yes>(std::forward<F>(callback));
}

} // namespace dmitigr::log

#endif  // DMITIGR_BASE_LOG_HPP
