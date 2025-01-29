// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
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
#include "../../prg/command.hpp"

#include <iostream>

#define ASSERT(a) DMITIGR_ASSERT(a)

int main(int argc, const char* const argv[])
{
  try {
    namespace prg = dmitigr::prg;
    ASSERT(argc);
    auto argc_copy = argc;
    auto argv_copy = argv;
    const auto cmd = prg::make_command(&argc_copy, &argv_copy, true);
    ASSERT(!argc_copy);
    ASSERT(argv_copy == argv + argc);
    const auto name = cmd.name();
    ASSERT(!name.empty());
    std::cout << "Command: " << name << std::endl;

    // Print all passed options.
    {
      const auto& opts = cmd.options();
      std::cout << opts.size() << " options specified";
      if (!opts.empty()) {
        std::cout << ":" << std::endl;
        for (const auto& o : opts) {
          std::cout << "  " << o.first;
          if (o.second)
            std::cout << " = " << *o.second;
          std::cout << std::endl;
        }
      } else
        std::cout << "." << std::endl;
    }

    // Print all passed command parameters.
    {
      const auto& params = cmd.parameters();
      std::cout << params.size() << " parameters specified";
      if (!params.empty()) {
        std::cout << ":" << std::endl;
        for (const auto& param : params)
          std::cout << "  " << param << std::endl;
      } else
        std::cout << "." << std::endl;
    }

    // Check valid options: --opt1, --opt2, --opt3.
    try {
      const auto [o1, o2, o3] = cmd.options_strict("opt1", "opt2", "opt3");
      [](const auto... opts)
      {
        ([opts]
        {
          std::cout << "--"<<opts.name();
          if (opts)
            std::cout <<": " << opts.value()
              .value_or("value not specified");
          else
            std::cout <<": option not specified";
          std::cout << std::endl;
        }(), ...);
      }(o1, o2, o3);
    } catch (const std::runtime_error& e) {
      std::cerr << e.what()
                << ". Valid options are: --opt1, --opt2, --opt3" << std::endl;
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
