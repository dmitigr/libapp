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

#pragma once

#include "windows.hpp"

#include <string>

namespace dmitigr::winbase {

inline std::wstring load_wstring(const UINT id, const HINSTANCE instance = {})
{
  LPWSTR ptr{};
  const int r{LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&ptr), 0)};
  return std::wstring(ptr, r);
}

} // namespace dmitigr::winbase
