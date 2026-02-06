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

#if !defined(__linux__) && !defined(__APPLE__)
#error dmitigr/nix/detach.hpp is usable only on Linux or macOS!
#endif

#ifndef DMITIGR_NIX_DETACH_HPP
#define DMITIGR_NIX_DETACH_HPP

#include "../base/assert.hpp"
#include "../base/filesystem.hpp"
#include "../base/log.hpp"
#include "error.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <functional>
#include <string>
#include <system_error>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace dmitigr::nix {

/**
 * @brief Detaches the process to make it work in background.
 *
 * @param startup The startup function to be called from a forked process.
 * @param pid_file The PID file that will be created and to which the
 * ID of the forked process will be written.
 * @param log_file The log file the detached process will use as the
 * destination instead of `std::clog` to write the log info.
 * @param log_file_openmode The openmode to use upon opening the specified
 * `log_file`.
 *
 * @par Requires
 * `(startup && !working_directory.empty() && !pid_file.empty() && !log_file.empty)`.
 *
 * @remarks The function returns in the detached (forked) process!
 */
inline void detach(const std::function<void()>& startup,
  const std::filesystem::path& working_directory,
  const std::filesystem::path& pid_file,
  const std::filesystem::path& log_file,
  const std::ios_base::openmode log_file_openmode =
  std::ios_base::app | std::ios_base::ate | std::ios_base::out)
{
  namespace fsx = dmitigr::filesystem;
  DMITIGR_ASSERT(startup);
  if (working_directory.empty()) {
    log::cerr("cannot detach process because the working directory isn't "
      "specified\n");
    std::exit(EXIT_SUCCESS);
  }
  if (pid_file.empty() || pid_file == "." || pid_file == "..") {
    log::cerr("cannot detach process because the PID file name is invalid\n");
    std::exit(EXIT_SUCCESS);
  }
  if (log_file.empty() || log_file == "." || log_file == "..") {
    log::cerr("cannot detach process because the log file name is invalid\n");
    std::exit(EXIT_SUCCESS);
  }

  // Forking for a first time
  if (const auto pid = ::fork(); pid < 0) {
    const int err = errno;
    log::cerr("first fork() failed ({})\n", error_message(err));
    std::exit(EXIT_FAILURE); // exit parent
  } else if (pid > 0)
    std::exit(EXIT_SUCCESS); // exit parent

  // Setting the umask for a new child process
  ::umask(S_IWGRP | S_IRWXO);

  // Redirecting stderr streams to `log_file`.
  try {
    log::redirect(log_file, log_file_openmode);
  } catch (const std::exception& e) {
    log::cerr("{}\n", e.what());
    std::exit(EXIT_FAILURE); // exit parent
  } catch (...) {
    log::cerr("cannot redirect stderr streams to {}\n", log_file.string());
    std::exit(EXIT_FAILURE); // exit parent
  }

  // Setup the new process group leader
  if (const auto sid = ::setsid(); sid < 0) {
    const int err = errno;
    log::cerr("cannot setup the new process group leader ({})\n",
      error_message(err));
    std::exit(EXIT_FAILURE);
  }

  // Forking for a second time
  if (const auto pid = ::fork(); pid < 0) {
    const int err = errno;
    log::cerr("second fork() failed ({})\n", error_message(err));
    std::exit(EXIT_FAILURE);
  } else if (pid > 0)
    std::exit(EXIT_SUCCESS);

  // Creating the PID file
  try {
    fsx::overwrite(pid_file, std::to_string(getpid()));
  } catch (const std::exception& e) {
    log::cerr("{}\n", e.what());
    std::exit(EXIT_FAILURE);
  } catch (...) {
    log::cerr("cannot open log file at {}\n", log_file.string());
    std::exit(EXIT_FAILURE);
  }

  // Changing the CWD
  try {
    std::filesystem::current_path(working_directory);
  } catch (const std::exception& e) {
    log::cerr("{}\n", e.what());
    std::exit(EXIT_FAILURE);
  } catch (...) {
    log::cerr("cannot change current working directory to {}\n",
      working_directory.string());
    std::exit(EXIT_FAILURE);
  }

  // Closing the standard file descriptors
  static auto close_fd = [](const int fd)
  {
    if (::close(fd)) {
      const int err = errno;
      log::cerr("cannot close file descriptor {}: {}\n", fd, error_message(err));
      std::exit(EXIT_FAILURE);
    }
  };

  close_fd(STDIN_FILENO);
  close_fd(STDOUT_FILENO);
  close_fd(STDERR_FILENO);

  // Starting up.
  try {
    startup();
  } catch (const std::exception& e) {
    log::cerr("{}\n", e.what());
    std::exit(EXIT_FAILURE);
  } catch (...) {
    log::cerr("start routine failed\n");
    std::exit(EXIT_FAILURE);
  }
}

/**
 * @brief Calls `startup` in the current process.
 *
 * @param detach Denotes should the process be forked or not.
 * @param startup A function to call. This function is called in a current
 * process if `!detach`, or in a forked process otherwise
 * @param working_directory A path to a new working directory. If not specified
 * the directory of executable is assumed.
 * @param pid_file A path to a PID file. If not specified the name of executable
 * with ".pid" extension in the working directory is assumed.
 * @param log_file A path to a log file. If not specified the name of executable
 * with ".log" extension in the working directory is assumed.
 * @param log_file_mode A file mode for the log file.
 *
 * @par Requires
 * `startup`.
 */
inline void start(const bool detach,
  void(*startup)(),
  std::filesystem::path executable,
  std::filesystem::path working_directory = {},
  std::filesystem::path pid_file = {},
  std::filesystem::path log_file = {},
  const std::ios_base::openmode log_file_mode =
    std::ios_base::trunc | std::ios_base::out)
{
  DMITIGR_ASSERT(startup);
  DMITIGR_ASSERT(!executable.empty());

  const auto run = [&startup]
  {
    try {
      startup();
    } catch (const std::exception& e) {
      log::cerr("{}\n", e.what());
      std::exit(EXIT_FAILURE);
    } catch (...) {
      log::cerr("unknown error\n");
      std::exit(EXIT_FAILURE);
    }
  };

  // Preparing.

  namespace fs = std::filesystem;
  namespace fsx = dmitigr::filesystem;
  if (working_directory.empty())
    working_directory = executable.parent_path();

  if (detach) {
    if (pid_file.empty()) {
      pid_file = working_directory / executable.filename();;
      pid_file.replace_extension(".pid");
    }
    if (log_file.empty()) {
      log_file = working_directory / executable.filename();
      log_file.replace_extension(".log");
    }
    fs::create_directories(pid_file.parent_path());
    fs::create_directories(log_file.parent_path());
  }

  // Starting.

  if (!detach) {
    DMITIGR_ASSERT(!working_directory.empty());
    std::error_code errc;
    fs::current_path(working_directory, errc);
    if (errc) {
      std::cerr << "cannot change the working directory to "
                << working_directory.string() + ": " + errc.message() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    if (!pid_file.empty())
      fsx::overwrite(pid_file, std::to_string(getpid()));

    if (!log_file.empty())
      log::redirect(log_file, log_file_mode);

    run();
  } else
    nix::detach(run, working_directory, pid_file, log_file, log_file_mode);
}

} // namespace dmitigr::nix

#endif  // DMITIGR_NIX_DETACH_HPP
