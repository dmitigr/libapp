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

#ifndef DMITIGR_WEB_UTIL_HPP
#define DMITIGR_WEB_UTIL_HPP

#include <string>
#include <string_view>

namespace dmitigr::web {

/// @returns An escaped HTML.
inline std::string esc(const std::string_view input)
{
  std::string result;
  for (const auto ch : input) {
    if (ch == '<')
      result += "&lt;";
    else if (ch == '>')
      result += "&gt;";
    else
      result += ch;
  }
  return result;
}

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_UTIL_HPP
