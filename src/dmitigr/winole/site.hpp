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
#include "../winbase/windows.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <stdexcept>

#include <comdef.h> // avoid LNK2019
#include <Oleidl.h>

namespace dmitigr::winole {

class Site final : public IOleClientSite
                 , public IOleInPlaceSite
                 , public IOleInPlaceFrame
                 , private Noncopymove {
public:
  // ---------------------------------------------------------------------------
  // Messages
  // ---------------------------------------------------------------------------

  enum Wm : UINT {
    wm_activate_in_place = WM_APP + 1
  };

  // ---------------------------------------------------------------------------
  // WndProc
  // ---------------------------------------------------------------------------

  struct Wm_create_lparam final {
    IUnknown* com{};
  };

  static LRESULT CALLBACK wnd_proc(HWND window,
    UINT message, WPARAM wparam, LPARAM lparam);

  // ---------------------------------------------------------------------------
  // Class API
  // ---------------------------------------------------------------------------

  ~Site()
  {
    try {
      try {
        deactivate_in_place();
      } catch (...){}

      if (ole_in_place_active_object_) {
        ole_in_place_active_object_->Release();
        ole_in_place_active_object_ = {};
      }

      assert(ole_object_);
      ole_object_->Release();
      ole_object_ = {};
    } catch (...) {}
  }

  explicit Site(IUnknown* const com, const HWND window)
    : window_{window}
  {
    if (!com || !window_ || !GetParent(window))
      throw std::invalid_argument{"cannot create OLE site instance"};

    if (com->QueryInterface(&ole_object_) != S_OK)
      throw std::runtime_error{"cannot query IOleObject "
        "upon of OLE site instance creation"};

    if (ole_object_->SetClientSite(this) != S_OK)
      throw std::runtime_error{"cannot inform IOleObject of its display location "
        "upon of OLE site instance creation"};
    if (OleSetContainedObject(ole_object_, true) != S_OK)
      throw std::runtime_error{"cannot notify IOleObject that it is embedded in "
        "an OLE container upon of OLE site instance creation"};

    assert(window_ && ole_object_);
  }

  HWND window() const noexcept
  {
    return window_;
  }

  IOleObject* ole_object() const noexcept
  {
    return ole_object_;
  }

  void set_ole_in_place_active_object(IOleInPlaceActiveObject* const obj)
  {
    ole_in_place_active_object_ = obj;
  }

  IOleInPlaceActiveObject* ole_in_place_active_object() const noexcept
  {
    return ole_in_place_active_object_;
  }

  void activate_in_place()
  {
    if (is_in_place_activated_)
      return;

    RECT rect{};

    if (!GetClientRect(window_, &rect))
      throw std::runtime_error{"cannot activate OLE site in place"};

    if (ole_object()->DoVerb(OLEIVERB_INPLACEACTIVATE,
        0, this, 0, window_, &rect) != S_OK)
      throw std::runtime_error{"cannot activate OLE site in place"};

    if (!InvalidateRect(window_, 0, true))
      throw std::runtime_error{"cannot activate OLE site in place"};

    is_in_place_activated_ = true;
  }

  void deactivate_in_place()
  {
    if (!is_in_place_activated_)
      return;

    const char* errmsg{};

    {
      IOleInPlaceObject* in_place_obj{};
      if (ole_object()->QueryInterface(&in_place_obj) != S_OK)
        errmsg = "cannot query interface upon deactivation of OLE site in place";

      if (!errmsg && in_place_obj->UIDeactivate() != S_OK)
        errmsg = "cannot deactivate UI upon of deactivation of OLE site in place";

      if (!errmsg && in_place_obj->InPlaceDeactivate() != S_OK)
        errmsg ="cannot deactivate in place upon of deactivation of OLE site in place";

      in_place_obj->Release();
    }

    if (!errmsg && !InvalidateRect(window_, 0, true))
      errmsg = "cannot invalidate site window rect upon of deactivation of OLE site"
        " in place";

    if (errmsg)
      throw std::runtime_error{errmsg};

    is_in_place_activated_ = false;
  }

  // ---------------------------------------------------------------------------
  // IUnknown overrides
  // ---------------------------------------------------------------------------

  HRESULT QueryInterface(REFIID iid, void** const object) override
  {
    if (iid == __uuidof(IUnknown))
      *object = static_cast<IUnknown*>(static_cast<IOleClientSite*>(this));
    else if (iid == __uuidof(IOleClientSite))
      *object = static_cast<IOleClientSite*>(this);
    else if (iid == __uuidof(IOleInPlaceSite))
      *object = static_cast<IOleInPlaceSite*>(this);
    else if (iid == __uuidof(IOleInPlaceFrame))
      *object = static_cast<IOleInPlaceFrame*>(this);
    else {
      *object = nullptr;
      return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
  }

  ULONG AddRef() override
  {
    return ++ref_count_;
  }

  ULONG Release() override
  {
    return ref_count_ = std::max(--ref_count_, ULONG(0));
  }

  // ---------------------------------------------------------------------------
  // IOleClientSite overrides
  // ---------------------------------------------------------------------------

  HRESULT SaveObject() override
  {
    return S_OK;
  }

  HRESULT GetMoniker(DWORD, DWORD, IMoniker** const moniker) override
  {
    *moniker = nullptr;
    return E_NOTIMPL;
  }

  HRESULT GetContainer(IOleContainer** const container) override
  {
    *container = nullptr;
    return E_FAIL;
  }

  HRESULT ShowObject() override
  {
    return S_OK;
  }

  HRESULT OnShowWindow(BOOL) override
  {
    InvalidateRect(window_, 0, true);
    InvalidateRect(GetParent(window_), 0, true);
    return S_OK;
  }

  HRESULT RequestNewObjectLayout() override
  {
    return S_OK;
  }

  // ---------------------------------------------------------------------------
  // IOleInPlaceSite overrides
  // ---------------------------------------------------------------------------

  HRESULT GetWindow(HWND* const window) override
  {
    *window = window_;
    return S_OK;
  }

  HRESULT ContextSensitiveHelp(BOOL) override
  {
    return E_NOTIMPL;
  }

  HRESULT CanInPlaceActivate() override
  {
    return S_OK;
  }

  HRESULT OnInPlaceActivate() override
  {
    return S_OK;
  }

  HRESULT OnUIActivate() override
  {
    return S_OK;
  }

  HRESULT GetWindowContext(IOleInPlaceFrame** const frame,
    IOleInPlaceUIWindow** const window,
    const LPRECT rect1, const LPRECT rect2,
    const LPOLEINPLACEFRAMEINFO finfo) override
  {
    *frame = static_cast<IOleInPlaceFrame*>(this);
    AddRef();

    *window = nullptr;
    GetClientRect(window_, rect1);
    GetClientRect(window_, rect2);
    finfo->cb = sizeof(OLEINPLACEFRAMEINFO);
    finfo->fMDIApp = false;
    finfo->hwndFrame = GetParent(window_);
    finfo->haccel = 0;
    finfo->cAccelEntries = 0;

    return S_OK;
  }

  HRESULT Scroll(SIZE) override
  {
    return E_NOTIMPL;
  }

  HRESULT OnUIDeactivate(int) override
  {
    return S_OK;
  }

  HRESULT OnInPlaceDeactivate() override
  {
    return S_OK;
  }

  HRESULT DiscardUndoState() override
  {
    return S_OK;
  }

  HRESULT DeactivateAndUndo() override
  {
    return S_OK;
  }

  HRESULT OnPosRectChange(LPCRECT) override
  {
    return S_OK;
  }

  // ---------------------------------------------------------------------------
  // IOleInPlaceFrame overrides
  // ---------------------------------------------------------------------------

  HRESULT GetBorder(const LPRECT result) override
  {
    GetClientRect(window_, result);
    return S_OK;
  }

  HRESULT RequestBorderSpace(LPCBORDERWIDTHS) override
  {
    return E_NOTIMPL;
  }

  HRESULT SetBorderSpace(LPCBORDERWIDTHS) override
  {
    return S_OK;
  }

  HRESULT SetActiveObject(IOleInPlaceActiveObject* const obj, LPCOLESTR) override
  {
    set_ole_in_place_active_object(obj);
    return S_OK;
  }

  HRESULT InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) override
  {
    return E_NOTIMPL;
  }

  HRESULT SetMenu(HMENU, HOLEMENU, HWND) override
  {
    return E_NOTIMPL;
  }

  HRESULT RemoveMenus(HMENU) override
  {
    return E_NOTIMPL;
  }

  HRESULT SetStatusText(LPCOLESTR) override
  {
    return E_NOTIMPL;
  }

  HRESULT EnableModeless(BOOL) override
  {
    return E_NOTIMPL;
  }

  HRESULT TranslateAccelerator(LPMSG, WORD) override
  {
    return E_NOTIMPL;
  }

private:
  ULONG ref_count_{};
  HWND window_{};
  IOleObject* ole_object_{};
  IOleInPlaceActiveObject* ole_in_place_active_object_{};
  bool is_in_place_activated_{};
};

// -----------------------------------------------------------------------------
// WndProc
// -----------------------------------------------------------------------------

inline LRESULT CALLBACK Site::wnd_proc(const HWND window, const UINT message,
  const WPARAM wparam, const LPARAM lparam)
{
  static const auto ole_site = [](const HWND window)noexcept
  {
    return reinterpret_cast<Site*>(GetWindowLongPtr(window, GWLP_USERDATA));
  };

  switch (message) {
  case WM_CREATE:
    try {
      if (!lparam)
        return -1;

      const auto* const awl = static_cast<Wm_create_lparam*>(
        reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);

      auto site = std::make_unique<Site>(awl->com, window);
      SetWindowLongPtrW(window, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(site.release()));
    } catch (...) {
      return -1;
    }
    break;

  case WM_DESTROY:
    {
      std::unique_ptr<Site> site{ole_site(window)};
      SetWindowLongPtrW(window, GWLP_USERDATA, 0);
      site.reset();
    }
    break;

  case WM_SIZE:
    {
      auto* const site = ole_site(window);
      assert(site);

      // Notify a parent window.
      {
        NMHDR nh{};
        nh.hwndFrom = window;
        nh.idFrom = GetWindowLong(window, GWL_ID);
        nh.code = 1;
        SendMessage(GetParent(window), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nh));
      }

      IOleInPlaceObject* in_place_obj{};
      if (site->ole_object()->QueryInterface(&in_place_obj) != S_OK)
        return 0;

      RECT rect{};
      GetClientRect(window, &rect);
      in_place_obj->SetObjectRects(&rect, &rect);
      in_place_obj->Release();
    }
    break;

  case wm_activate_in_place:
    {
      auto* const site = ole_site(window);
      assert(site);

      if (wparam)
        site->activate_in_place();
      else
        site->deactivate_in_place();
    }
    break;

  default:
    return DefWindowProc(window, message, wparam, lparam);
  }

  return 0;
}

} // namespace dmitigr::winole
