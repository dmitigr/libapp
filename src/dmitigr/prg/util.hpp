// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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

#ifndef DMITIGR_PRG_UTIL_HPP
#define DMITIGR_PRG_UTIL_HPP

#include "info.hpp"

#include <csignal>
#include <cstdlib>
#include <ostream>
#include <string_view>

namespace dmitigr::prg {

/**
 * @brief Prints the usage info on the standard error and terminates the
 * program with unsuccessful exit code.
 *
 * @par Requires
 * `Info::is_initialized()`.
 */
[[noreturn]] inline void exit_usage(std::ostream& out,
  const int code = EXIT_FAILURE)
{
  const auto& info = Info::instance();
  out << "usage: " << info.program_name();
  const auto synop = info.synopsis();
  if (!synop.empty())
    out << " " << synop;
  out << std::endl;
  std::exit(code);
}

// =============================================================================

/// A typical signal handler.
inline void handle_signal(const int sig) noexcept
{
  Info::instance().stop_signal = sig;
}

/// Assigns the `signals` as a signal handler of some signals.
inline void set_signals(void(*signals)(int) = &handle_signal) noexcept
{
  std::signal(SIGABRT, signals);
  std::signal(SIGFPE, signals);
  std::signal(SIGILL, signals);
  std::signal(SIGINT, signals);
  std::signal(SIGSEGV, signals);
  std::signal(SIGTERM, signals);
}

// =============================================================================

/**
 * @brief Calls the function `f`.
 *
 * @details If the call of `callback` fails with exception then
 * `Info::instance().stop_signal` flag is sets to `stop_signal`.
 *
 * @param f A function to call
 */
template<typename F>
auto with_signal_on_error(F&& f, const int stop_signal = SIGTERM)
{
  try {
    return f();
  } catch (...) {
    Info::instance().stop_signal = stop_signal;
    throw;
  }
}

} // namespace dmitigr::prg

#endif  // DMITIGR_PRG_UTIL_HPP
