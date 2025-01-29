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

#include "../base/noncopymove.hpp"
#include "windows.hpp"
#include "strconv.hpp"

#include <limits>
#include <stdexcept>
#include <string_view>

namespace dmitigr::winbase {

class Menu_guard final : private Noncopy {
public:
  ~Menu_guard()
  {
    if (handle_) {
      DestroyMenu(handle_);
      handle_ = {};
    }
  }

  Menu_guard() = default;

  explicit Menu_guard(HMENU handle)
    : handle_{handle}
  {}

  Menu_guard(Menu_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = {};
  }

  Menu_guard& operator=(Menu_guard&& rhs) noexcept
  {
    Menu_guard tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Menu_guard& rhs) noexcept
  {
    using std::swap;
    swap(handle_, rhs.handle_);
  }

  HMENU release() noexcept
  {
    auto result = handle_;
    handle_ = {};
    return result;
  }

  HMENU handle() const noexcept
  {
    return handle_;
  }

  explicit operator bool() const noexcept
  {
    return static_cast<bool>(handle());
  }

private:
  HMENU handle_{};
};

inline void append_menu_item(const HMENU menu, const std::string_view text,
  const UINT id, const UINT state, const HMENU submenu = {})
{
  auto mitem = utf8_to_utf16(text);
  MENUITEMINFOW mii{};
  if constexpr (sizeof(decltype(mitem.size())) > sizeof(decltype(mii.cch))) {
    if (mitem.size() > std::numeric_limits<decltype(mii.cch)>::max())
      throw std::runtime_error{"cannot append menu item"};
  }
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
  if ( (mii.hSubMenu = submenu))
    mii.fMask |= MIIM_SUBMENU;
  mii.wID = id;
  mii.fState = state;
  mii.dwTypeData = mitem.data();
  mii.cch = static_cast<decltype(mii.cch)>(mitem.size());
  InsertMenuItemW(menu, GetMenuItemCount(menu), true, &mii);
}

} // namespace dmitigr::winbase
