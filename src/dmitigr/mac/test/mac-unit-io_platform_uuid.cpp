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
#include "../io.hpp"

int main()
{
  using std::cout;
  using std::cerr;
  using std::endl;
  try {
    namespace mac = dmitigr::mac;

    cout << mac::io::platform_uuid() << endl;

  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  } catch (...) {
    cerr << "unknown error" << endl;
    return 2;
  }
}
