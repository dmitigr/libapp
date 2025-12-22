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

#ifndef DMITIGR_PRG_UTIL_HPP
#define DMITIGR_PRG_UTIL_HPP

#include "info.hpp"

#include <csignal>
#include <cstdlib>
#include <ostream>
#include <iterator>
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
  if (const auto synop = info.synopsis(); !synop.empty()) {
    out << "usage: " << info.program_name() << " " << synop.front() << '\n';
    const auto e = synop.cend();
    for (auto i = next(synop.cbegin()); i != e; ++i)
      out << "       " << info.program_name() << " " << *i << '\n';
    out << std::flush;
  }
  std::exit(code);
}

/**
 * @brief Calls `callback` and if it throws an exception prints the exception's
 * what-string to `out` and calls exit_usage(out, code).
 */
template<typename F>
auto with_exit_usage_on_throw(F&& callback, std::ostream& out,
  const int code = EXIT_FAILURE)
{
  try {
    return callback();
  } catch (const std::exception& e) {
    out << e.what() << '\n';
    exit_usage(out, code);
  } catch (...) {
    out << "unknown error\n";
    exit_usage(out, code);
  }
}

} // namespace dmitigr::prg

#endif  // DMITIGR_PRG_UTIL_HPP
