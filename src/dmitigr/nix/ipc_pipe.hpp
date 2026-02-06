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

#ifdef _WIN32
#error dmitigr/os/ipc_pipe.hpp is not usable on Microsoft Windows!
#endif

#ifndef DMITIGR_NIX_IPC_PIPE_HPP
#define DMITIGR_NIX_IPC_PIPE_HPP

#include "../base/str.hpp"
#include "error.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <poll.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/types.h>
#include <sys/uio.h>
#endif
#include <sys/wait.h>

namespace dmitigr::nix::ipc::pp {

/**
 * @brief Executes a `prog`.
 *
 * @param prog Executable name.
 * @param args Program arguments. (args[0] must be a program name!)
 * @param prog_stdin A data to be passed to the standard input of a program.
 * @param poll_timeout A duration that underlying polling routine should wait
 * for the standard output and/or standard error to become available.
 * @param stdout_handler A handler which is called when a portion of the standard
 * output of `prog` is available.
 * @param stderr_handler A handler which is called when a portion of the standard
 * error of `prog` is available.
 * @param progress_handler A handler which is called periodically upon waiting of
 * `prog` to finish.
 *
 * @details If any handler throws an exception, the attempt will be made to
 * terminate the `prog` with SIGKILL signal and wait for `prog` to finish.
 *
 * @par Requires
 * `poll_timeout >= 0`.
 *
 * @returns wstatus (see wait(2) manual page).
 */
inline int exec_and_wait(const std::string& prog,
  const std::vector<std::string>& args,
  const std::string_view prog_stdin,
  const std::chrono::milliseconds poll_timeout,
  const std::function<void(pid_t, std::string_view)>& stdout_handler,
  const std::function<void(pid_t, std::string_view)>& stderr_handler,
  const std::function<void(pid_t)>& progress_handler)
{
  if (!(poll_timeout >= std::chrono::milliseconds::zero()))
    throw std::runtime_error{"invalid polling timeout"};

  // Pipes (0 - r, 1 - w):
  int fildes_to_child_in[2];    // for writing `prog_stdin` to child's stdin;
  int fildes_from_child_out[2]; // for reading child's stdout;
  int fildes_from_child_err[2]; // for reading child's stderr.
  if (pipe(fildes_to_child_in) ||
    pipe(fildes_from_child_out) || pipe(fildes_from_child_err))
    throw std::runtime_error{"pipe() failed"};

  if (const auto pid = fork(); pid < 0)
    throw std::runtime_error{"fork() failed"};
  else if (!pid) { // child
    close(fildes_to_child_in[1]);
    close(fildes_from_child_out[0]);
    close(fildes_from_child_err[0]);

    if (dup2(fildes_to_child_in[0], STDIN_FILENO) < 0 ||
      dup2(fildes_from_child_out[1], STDOUT_FILENO) < 0 ||
      dup2(fildes_from_child_err[1], STDERR_FILENO) < 0)
      std::exit(1);

    close(fildes_to_child_in[0]);
    close(fildes_from_child_out[1]);
    close(fildes_from_child_err[1]);

    // exec*() return only if an error has occurred.
    const auto argv = str::vector_c_str(args);
    (void)execvp(prog.c_str(), const_cast<char**>(argv.data()));
    const int err{errno};
    std::fprintf(stderr, "%s", error_message(err).c_str());
    std::exit(1);
  } else { // parent
    close(fildes_to_child_in[0]);
    close(fildes_from_child_out[1]);
    close(fildes_from_child_err[1]);

    if (!prog_stdin.empty())
      write(fildes_to_child_in[1], prog_stdin.data(), prog_stdin.size());

    close(fildes_to_child_in[1]);

    struct Guard final {
      ~Guard()
      {
        close(fildes_from_child_out);
        close(fildes_from_child_err);
      }
      const int fildes_from_child_out{};
      const int fildes_from_child_err{};
    } const guard{fildes_from_child_out[0], fildes_from_child_err[0]};

    const auto read_from_child = [pid](const int fd, std::string& buf, const auto& handler)
    {
      buf.resize(16384);
      if (const ssize_t count{::read(fd, buf.data(), buf.size())}; count > 0 && handler) {
        buf.resize(count);
        handler(pid, buf);
      }
    };
    int wstatus;
    std::string stdout_buffer;
    std::string stderr_buffer;
    while (true) {
      try {
        // Read from child.
        pollfd fds[] = {
          {fildes_from_child_out[0], POLLIN, 0},
          {fildes_from_child_err[0], POLLIN, 0}
        };
        poll(fds, sizeof(fds) / sizeof(*fds), poll_timeout.count());
        if (bool(fds[0].revents & POLLIN))
          read_from_child(fds[0].fd, stdout_buffer, stdout_handler);
        if (bool(fds[1].revents & POLLIN))
          read_from_child(fds[1].fd, stderr_buffer, stderr_handler);

        /*
         * Wait child to change state. Break only if
         * it's exited either voluntarily or forcibly.
         */
        if (const pid_t p{waitpid(pid, &wstatus, WNOHANG)}) {
          if (p == -1 || WIFEXITED(wstatus))
            break;
        }

        if (progress_handler)
          progress_handler(pid);
      } catch (...) {
        if (!kill(pid, SIGKILL))
          waitpid(pid, &wstatus, 0);
        throw;
      }
    }
    return wstatus;
  }
}

/// @overload
inline int exec_and_wait(const std::string& prog,
  const std::vector<std::string>& args,
  const std::string_view prog_stdin,
  std::ostream& stdout,
  std::ostream& stderr)
{
  return exec_and_wait(prog, args, prog_stdin,
    std::chrono::seconds{1},
    [&stdout](const pid_t, const std::string_view out)
    {
      stdout << out;
    },
    [&stderr](const pid_t, const std::string_view err)
    {
      stderr << err;
    },
    [](const pid_t){});
}

} // namespace dmitigr::nix::ipc::pp

#endif  // DMITIGR_NIX_IPC_PIPE_HPP
