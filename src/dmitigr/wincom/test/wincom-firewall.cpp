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

#include "../../base/assert.hpp"
#include "../firewall.hpp"
#include "../library.hpp"

#include <iostream>

#define ASSERT DMITIGR_ASSERT

int main()
{
  using std::cout;
  using std::clog;
  using std::wcout;
  using std::endl;
  namespace com = dmitigr::wincom;

  try {
    const com::Library library;
    const auto rules = com::firewall::Policy2{}.rules();
    const auto rule = rules.rule(std::string{"@FirewallAPI.dll,-28778"});
    if (rule) {
      cout << rule.description<std::string>() << endl;
      cout << "enabled="<<rule.is_enabled() << endl;
    } else
      cout << "rule not found" << endl;
  } catch (const std::exception& e) {
    clog << "error: " << e.what() << endl;
    return 1;
  } catch (...) {
    clog << "unknown error" << endl;
    return 2;
  }
}
