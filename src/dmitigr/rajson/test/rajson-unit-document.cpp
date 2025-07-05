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
#include "../../rajson/document.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#define ASSERT DMITIGR_ASSERT

namespace fs = std::filesystem;
namespace rajson = dmitigr::rajson;
namespace rajson = dmitigr::rajson;

struct Config final {
  Config() = default;

  Config(const std::string_view input)
    : cfg{input}
  {
    // host.
    cfg.get(host, "host");
    if (host.empty() || any_of(cbegin(host),
        cend(host), [](const auto c){return std::isspace(c);}))
      throw std::runtime_error{"invalid host config parameter"};

    // port.
    cfg.get(port, "port");
    if (port < 0 || port > 65535)
      throw std::runtime_error{"invalid port config parameter"};

    // dir/log.
    cfg.get(dir_log, rajson::Path{"dir/log"});
    if (dir_log.empty())
      throw std::runtime_error{"invalid log dir"};

    // dir/wrk.
    cfg.get(dir_wrk, rajson::Path{"dir/wrk"});
    if (dir_wrk.empty())
      throw std::runtime_error{"invalid wrk dir"};
  }

  rajson::Document cfg;
  std::string host;
  int port{};
  fs::path dir_log;
  fs::path dir_wrk;
};

int main()
try {
  using namespace std::literals::string_view_literals;

  Config cfg;
  ASSERT(cfg.cfg.json().value().IsNull());

  cfg = Config{R"({"host":"localhost", "port":80,
"dir":{"log":"logdir", "wrk":"wrkdir"}})"sv};
  ASSERT(cfg.host == "localhost");
  ASSERT(cfg.port == 80);
  ASSERT(cfg.dir_log == "logdir");
  ASSERT(cfg.dir_wrk == "wrkdir");
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}
