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

// The Windows Shell

#pragma once
#pragma comment(lib, "shell32")

#include "../base/traits.hpp"
#include "combase.hpp"
#include "exceptions.hpp"
#include "hlocal.hpp"
#include "strconv.hpp"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <shellapi.h>
#include <shlobj.h>

namespace dmitigr::winbase::shell {

inline std::filesystem::path known_folder_path(REFKNOWNFOLDERID rfid,
  const DWORD flags = {}, const HANDLE user = {})
{
  PWSTR path{};
  const HRESULT err{SHGetKnownFolderPath(rfid, flags, user, &path)};
  if (err != S_OK)
    throw std::runtime_error{"cannot get known folder path: error "+
      std::to_string(err)};
  return com::Taskmem<WCHAR>{path}.value();
}

template<class String, typename Ch>
std::vector<String> argc_argv_to_vector(const int argc, const Ch* const* argv)
{
  using C = std::decay_t<Ch>;
  using S = std::decay_t<String>;
  std::vector<String> result;
  result.reserve(argc);
  for (int i{}; i < argc; ++i) {
    using std::is_same_v;
    if constexpr (is_same_v<S, std::string>) {
      if constexpr (is_same_v<C, std::wstring::value_type>)
        result.push_back(utf16_to_utf8(argv[i]));
      else if constexpr (is_same_v<C, std::string::value_type>)
        result.emplace_back(argv[i]);
      else
        static_assert(false_value<C>, "unsupported type");
    } else if constexpr (std::is_same_v<S, std::wstring>) {
      if constexpr (is_same_v<C, std::string::value_type>)
        result.push_back(utf8_to_utf16(argv[i]));
      else if constexpr (is_same_v<C, std::wstring::value_type>)
        result.emplace_back(argv[i]);
      else
        static_assert(false_value<C>, "unsupported type");
    } else
      static_assert(false_value<S>, "unsupported type");
  }
  return result;
}

template<class String>
std::vector<String> command_line_to_vector(const LPCWSTR cmd_line)
{
  int argc{};
  Hlocal_guard argv{CommandLineToArgvW(cmd_line, &argc)};
  if (!argv)
    throw Sys_exception{"cannot parse command line string"};
  return argc_argv_to_vector<String>(argc, static_cast<LPWSTR*>(argv.handle()));
}

/// @overload
template<class String>
std::vector<String> command_line_to_vector(const std::wstring& cmd_line)
{
  return command_line_to_vector<String>(cmd_line.c_str());
}

} // dmitigr::winbase::shell
