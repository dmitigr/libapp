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

#pragma once

#include "process.hpp"
#include "processenv.hpp"
#include "shell.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace dmitigr::winbase {

/// A program.
class Program final {
public:
  /// Not constructible.
  Program() = delete;

  /// Initialize instance.
  static void init(const std::vector<std::string>& argv)
  {
    check_initialized(false);

    if (argv.empty())
      throw std::invalid_argument{"invalid argv"};

    argv_ = argv;
    path_ = module_filename();

    check_initialized(true);
  }

  /// @overload
  static void init()
  {
    init(shell::command_line_to_vector<std::string>(GetCommandLineW()));
  }

  /// @overload
  static void init(const int argc, const char* const* argv)
  {
    init(shell::argc_argv_to_vector<std::string>(argc, argv));
  }

  /// @returns `true` if the instance is initialized.
  static bool is_initialized() noexcept
  {
    return !path_.empty();
  }

  /// @returns Command line.
  static const std::vector<std::string>& argv()
  {
    check_initialized(true);
    return argv_;
  }

  /// @returns argv().
  static const std::vector<std::string>& command_line()
  {
    return argv();
  }

  /// @returns Executable path.
  static const std::filesystem::path& path()
  {
    check_initialized(true);
    return path_;
  }

private:
  static inline std::vector<std::string> argv_;
  static inline std::filesystem::path path_;

  static void check_initialized(const bool is_initialized)
  {
    const char* const msg = is_initialized ?
      "program not initialized" : "program already initialized";
    if (!(is_initialized ^ path_.empty()))
      throw std::logic_error{msg};
  }
};

} // namespace dmitigr::winbase
