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
#include "error.hpp"

#include <stdexcept>
#include <string>

namespace dmitigr::winbase {

struct Dialog final {
  DLGTEMPLATE tpl{};
  WORD menu{};
  WORD clss{};
  WORD title{};
};

inline std::wstring dialog_item_text(const HWND dlg, const int item_id,
  const UINT max_size)
{
  std::wstring result(max_size, '\0');
  const UINT result_size{GetDlgItemTextW(dlg, item_id,
      result.data(), result.size() + 1)};
  if (!result_size) {
    if (const auto err = GetLastError())
      throw std::runtime_error{system_message(err)};
  }
  result.resize(result_size);
  return result;
}

inline void set_dialog_item_text(const HWND dlg, const int item_id,
  const std::wstring& value)
{
  if (!SetDlgItemTextW(dlg, item_id, value.c_str()))
    throw std::runtime_error{last_error_message()};
}

} // namespace dmitigr::winbase
