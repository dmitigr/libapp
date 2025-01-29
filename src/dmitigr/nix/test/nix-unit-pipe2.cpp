// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin

#include "../../base/assert.hpp"
#include "../../nix/ipc_pipe.hpp"

#include <filesystem>

#include <sys/types.h>
#include <signal.h>

int main(const int, char* argv[])
{
  namespace pp = dmitigr::nix::ipc::pp;

  try {
    const auto exe = canonical(
      std::filesystem::path{argv[0]}).parent_path()/"dmitigr_nix-slow_write";
    const auto exe_string = exe.string();
    const char* const exe_cstr = exe_string.c_str();
    const auto r = pp::exec_and_wait(exe_cstr, {exe_cstr}, "",
      std::chrono::milliseconds{1000},
      [](const pid_t, const std::string_view out)
      {
        std::cout << "stdout: " << out << "(" << out.size() << ")" << std::endl;
      },
      [](const pid_t, const std::string_view err)
      {
        std::cerr << "stderr: " << err << "(" << err.size() << ")" << std::endl;
      },
      [count=0](const pid_t pid)mutable
      {
        std::cout << "process " << pid << " doing it's work, please wait..." << std::endl;
        if (count++ == 7)
          kill(pid, SIGTERM);
      });
    if (WIFEXITED(r)) {
      std::cout << "subprocess exited normally with status " << WEXITSTATUS(r) << std::endl;
      return WEXITSTATUS(r);
    } else if (WIFSIGNALED(r)) {
      std::cout << "subprocess exited by signal " << WTERMSIG(r) << std::endl;
      return 1;
    } else {
      std::cout << "subprocess exited, wstatus = " << r << std::endl;
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
