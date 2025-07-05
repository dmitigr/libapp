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

#pragma once
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "wbemuuid")

#include "../base/noncopymove.hpp"
#include "../winbase/combase.hpp"
#include "exceptions.hpp"
#include "object.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

#include <Wbemidl.h>

namespace dmitigr::wincom::wmi {

class Class_object final :
    public Unknown_api<Class_object, IWbemClassObject> {
  using Ua = Unknown_api<Class_object, IWbemClassObject>;
public:
  using Ua::Ua;

  struct Value final {
    CIMTYPE type{};
    winbase::com::Variant data;
  };

  Value value(const LPCWSTR name, long* const flavor = {}) const
  {
    if (!name)
      throw std::invalid_argument{"cannot get property of IWebClassObject:"
        " invalid name"};

    Value value;
    const auto err = detail::api(*this).Get(name, 0,
      &value.data.data(), &value.type, flavor);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get property "
      +winbase::utf16_to_utf8(name)+" of IWbemClassObject");
    return value;
  }
};

class Enum_class_object final :
    public Unknown_api<Enum_class_object, IEnumWbemClassObject> {
  using Ua = Unknown_api<Enum_class_object, IEnumWbemClassObject>;
public:
  using Ua::Ua;

  Class_object next(const long timeout = WBEM_INFINITE)
  {
    IWbemClassObject* result{};
    ULONG result_count{};
    const auto err = api().Next(timeout, 1, &result, &result_count);
    if (err == WBEM_S_NO_ERROR)
      return Class_object{result};
    else if (err != WBEM_S_FALSE)
      DMITIGR_WINCOM_THROW_IF_ERROR(err,
        "cannot get next object of IEnumWbemClassObject");
    return Class_object{};
  }
};

class Services final : public Unknown_api<Services, IWbemServices> {
  using Ua = Unknown_api<Services, IWbemServices>;
public:
  using Ua::Ua;

  template<class String>
  Enum_class_object exec_query(const String& query,
    const long flags = WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY,
    IWbemContext* const ctx = {}) const
  {
    IEnumWbemClassObject* result{};
    const auto err = detail::api(*this).ExecQuery(detail::bstr("WQL"),
      detail::bstr(query),
      flags,
      ctx,
      &result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot execute query to retrieve"
      " objects from WMI services");
    return Enum_class_object{result};
  }

  /**
   * @returns Object from the namespace associated with this instance.
   *
   * @param path Path of the object.
   * @param flags Flags affectings the behavior. Must not include
   * `WBEM_FLAG_RETURN_IMMEDIATELY`.
   * @param ctx Additional context information.
   */
  Class_object object(const BSTR path,
    const long flags = WBEM_FLAG_RETURN_WBEM_COMPLETE,
    IWbemContext* const ctx = {}) const
  {
    IWbemClassObject* result{};
    const auto err = detail::api(*this).GetObject(path, flags, ctx, &result,
      nullptr);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get object from WMI services");
    return Class_object{result};
  }
};

class Locator final : public Basic_com_object<WbemLocator, IWbemLocator> {
  using Bco = Basic_com_object<WbemLocator, IWbemLocator>;
public:
  using Bco::Bco;

  Services connect_server(const BSTR network_resource,
    const BSTR user,
    const BSTR password,
    const BSTR locale,
    const long security_flags,
    const BSTR authority,
    IWbemContext* const ctx = {})
  {
    IWbemServices* result{};
    const auto err = api().ConnectServer(
      network_resource,
      user,
      password,
      locale,
      security_flags,
      authority,
      ctx,
      &result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot connect to "
      +winbase::com::to_string(network_resource));
    return Services{result};
  }
};

} // namespace dmitigr::wincom::wmi
