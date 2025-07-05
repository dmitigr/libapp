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

#include "../../base/assert.hpp"
#include "../../nix/detach.hpp"

#include <csignal>
#include <iostream>

inline std::filesystem::path pid_file;
inline std::filesystem::path log_file;

int main(int, char* argv[])
{
  try {
    namespace nix = dmitigr::nix;

    const auto dirname = std::filesystem::path{argv[0]}.parent_path();
    pid_file = std::filesystem::absolute(dirname/"nix-detach.pid");
    log_file = std::filesystem::absolute(dirname/"nix-detach.log");
    nix::detach([]
    {
      static const auto cleanup = []
      {
        std::clog << "Cleaning up..." << std::endl;
        {
          std::clog << "Attempting to remove the PID file " << pid_file << "...";
          std::error_code e;
          if (std::filesystem::remove(pid_file, e))
            std::clog << "done" << std::endl;
          else
            std::clog << "failure" << std::endl;
        }
      };

      std::set_terminate(cleanup);
      std::atexit(cleanup);

      std::signal(SIGHUP,  &std::exit);
      std::signal(SIGINT,  &std::exit);
      std::signal(SIGTERM, &std::exit);

      std::clog << "Detached service done." << std::endl;
    }, dirname, pid_file, log_file);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
