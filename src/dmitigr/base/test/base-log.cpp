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

#define DMITIGR_LOG_WITH_LEVEL
#define DMITIGR_LOG_WITH_DEFAULT_PREFIX

#include "../../base/log.hpp"

int main()
{
  try {
    namespace log = dmitigr::log;
    tzset();
    log::level = log::Level::debug;
    log::emergency("emergency");
    log::alert("alert");
    log::critical("critical");
    log::error("error");
    log::warning("warning");
    log::notice("notice");
    log::info("info");
    log::debug("debug");

    log::call_nothrow<"test log::call">([]
    {
      throw std::runtime_error{"it's expected"};
    });
    log::call_nothrow<"test log::call", "failure to {}: {}", "{} {} ({})">([]
    {
      throw "it's expected";
    }, "additional info");
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
