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

// Extension of Windows.Data.Xml.Dom

#pragma once
#pragma comment(lib, "runtimeobject")

#include "../combase.hpp"
#include "../registry.hpp"
#include "../shell.hpp"
#include "data_xml_dom.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <knownfolders.h>
#include <propkey.h>

#include <windows.ui.notifications.h>
#include <wrl.h>

namespace dmitigr::winbase::rt::ui::notifications {

namespace base = ABI::Windows::UI::Notifications;
namespace wrl = Microsoft::WRL;

inline std::filesystem::path shortcut_path(
  const std::wstring& app_name,
  const std::filesystem::path& subpath,
  const bool is_for_current_user)
{
  const auto folder = is_for_current_user ? FOLDERID_Programs :
    FOLDERID_CommonPrograms;
  auto result = shell::known_folder_path(folder);
  if (!subpath.empty())
    result /= subpath;
  result /= app_name;
  return result.replace_extension("lnk");
}

inline void create_shortcut(const std::filesystem::path& shortcut,
  const std::filesystem::path& exepath,
  const std::wstring& app_name,
  REFCLSID activator)
{
  if (shortcut.empty())
    throw std::invalid_argument{"cannot create shortcut by using empty path"};
  else if (app_name.empty())
    throw std::invalid_argument{"cannot create shortcut by using empty"
      " application name"};

  wrl::ComPtr<IShellLink> shell_link;
  if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shell_link))))
    throw std::runtime_error{"cannot create instance of IShellLink"};

  {
    const auto wexepath = exepath.wstring();
    if (FAILED(shell_link->SetPath(wexepath.c_str())))
      throw std::runtime_error{"cannot set path of IShellLink"};
  }

  {
    const auto wdir = exepath.parent_path().wstring();
    if (FAILED(shell_link->SetWorkingDirectory(wdir.c_str())))
      throw std::runtime_error{"cannot set working directory of IShellLink"};
  }

  wrl::ComPtr<IPropertyStore> property_store;
  if (FAILED(shell_link.As(&property_store)))
    throw std::runtime_error{"cannot represent IShellLink as IPropertyStore"};

  {
    PROPVARIANT prop{};
    PropVariantClear(&prop);
    prop.vt = VT_LPWSTR;
    prop.pwszVal = const_cast<PWSTR>(app_name.c_str());
    if (FAILED(property_store->SetValue(PKEY_AppUserModel_ID, prop)))
      throw std::runtime_error{"cannot set PKEY_AppUserModel_ID"};
  }

  {
    PROPVARIANT prop{};
    PropVariantClear(&prop);
    prop.vt = VT_CLSID;
    prop.puuid = const_cast<CLSID*>(&activator);
    if (FAILED(property_store->SetValue(
          PKEY_AppUserModel_ToastActivatorCLSID, prop)))
      throw std::runtime_error{"cannot set PKEY_AppUserModel_ToastActivatorCLSID"};
  }

  if (FAILED(property_store->Commit()))
    throw std::runtime_error{"cannot commit the changes of IPropertyStore"};

  wrl::ComPtr<IPersistFile> persist_file;
  if (FAILED(shell_link.As(&persist_file)))
    throw std::runtime_error{"cannot represent IShellLink as IPersistFile"};

  {
    const auto wpath = shortcut.wstring();
    if (FAILED(persist_file->Save(wpath.c_str(), true)))
      throw std::runtime_error{"cannot save shortcut to "+shortcut.string()};
  }
}

inline void unregister_activator(REFCLSID activator,
  const std::wstring& app_name,
  const std::filesystem::path& subpath,
  const bool is_for_current_user)
{
  using registry::remove_key;
  const auto root = is_for_current_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
  remove_key(root, com::server_registry_localserver32(activator));
  remove_key(root, com::server_registry_root(activator));

  remove(shortcut_path(app_name, subpath, is_for_current_user));
}

inline void register_activator(REFCLSID activator,
  const std::wstring& app_name,
  const std::filesystem::path& subpath,
  const std::filesystem::path& exepath,
  const bool is_for_current_user,
  const std::wstring& handle_arg = L"handle")
{
  create_shortcut(shortcut_path(app_name, subpath, is_for_current_user),
    exepath, app_name, activator);

  const auto root = is_for_current_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
  const auto [key, disp] = registry::create_key(root,
    com::server_registry_localserver32(activator), KEY_WRITE);
  registry::set_value(key, L"", L"\""+exepath.wstring()+L"\" "
    + handle_arg);
}

/**
 * @brief Shows the specified `toast`.
 *
 * @warning The `toast` may not shown if an application exits immediately
 * after this function returns. Please, see
 * https://github.com/microsoft/WindowsAppSDK/issues/3437
 */
inline void show_toast(
  base::IToastNotificationManagerStatics& manager,
  const std::wstring& app_name,
  data::xml::dom::base::IXmlDocument& xml)
{
  if (app_name.empty())
    throw std::invalid_argument{"cannot show toast notification:"
      " empty application name"};

  wrl::ComPtr<base::IToastNotifier> notifier;
  if (FAILED(manager.CreateToastNotifierWithId(
    wrl::Wrappers::HStringReference(app_name.c_str()).Get(),
    &notifier)))
    throw std::runtime_error{"cannot create toast notifier"};

  wrl::ComPtr<base::IToastNotificationFactory> factory;
  if (FAILED(Windows::Foundation::GetActivationFactory(
    wrl::Wrappers::HStringReference(
      RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
    &factory)))
    throw std::runtime_error{"cannot get toast notification factory"};

  wrl::ComPtr<base::IToastNotification> toast;
  if (FAILED(factory->CreateToastNotification(&xml, &toast)))
    throw std::runtime_error{"cannot create toast notification"};
  if (FAILED(notifier->Show(toast.Get())))
    throw std::runtime_error{"cannot schedule toast notification"};
}

/// @overload
inline void show_toast(const std::wstring& app_name, const std::wstring& toast)
{
  wrl::ComPtr<base::IToastNotificationManagerStatics> manager;
  if (FAILED(Windows::Foundation::GetActivationFactory(
        wrl::Wrappers::HStringReference(
          RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
        &manager)))
    throw std::runtime_error{"cannot retrieve activation factory for"
      " RuntimeClass_Windows_UI_Notifications_ToastNotificationManager"};

  const auto doc = data::xml::dom::create_xml_document_from_string(toast);
  show_toast(*manager.Get(), app_name, *doc.Get());
}

} // dmitigr::winbase::rt::ui::notifications
