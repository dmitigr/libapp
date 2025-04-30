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
#pragma comment(lib, "ole32")

#include "../base/assert.hpp"
#include "../base/noncopymove.hpp"
#include "../base/traits.hpp"
#include "../winbase/windows.hpp"
#include "exceptions.hpp"

#include <comdef.h> // avoid LNK2019
#include <ocidl.h>
#include <Objbase.h>
#include <unknwn.h>
#include <WTypes.h> // MSHCTX, MSHLFLAGS

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <type_traits>

namespace dmitigr::wincom {

template<class T, class A>
T& query_interface(A& api)
{
  using D = std::decay_t<T>;
  if constexpr (!std::is_same_v<D, A>) {
    D* result{};
    api.QueryInterface(&result);
    if (!result)
      throw std::runtime_error{"cannot obtain interface "
        + std::string{typeid(D).name()}
        + " from "
        + std::string{typeid(A).name()}};
    return *result;
  } else
    return api;
}

// -----------------------------------------------------------------------------
// Unknown_api
// -----------------------------------------------------------------------------

template<class DerivedType, class ApiType>
class Unknown_api {
public:
  using Api = ApiType;
  using Derived = DerivedType;

  static_assert(std::is_base_of_v<IUnknown, Api>);

  template<class U>
  static Derived query(U* const unknown)
  {
    static_assert(std::is_base_of_v<IUnknown, U>);

    static const std::string msg{
      "cannot obtain interface "
      + std::string{typeid(Api).name()}
      + " from "
      + std::string{typeid(U).name()}
      + " to make "
      + std::string{typeid(Derived).name()}};

    if (!unknown)
      throw std::invalid_argument{msg+": null input pointer"};
    Api* api{};
    unknown->QueryInterface(&api);
    if (!api)
      throw std::runtime_error{msg};
    return Derived{api};
  }

  virtual ~Unknown_api()
  {
    if (api_) {
      api_->Release();
      api_ = nullptr;
    }
  }

  Unknown_api() = default;

  explicit Unknown_api(Api* const api)
    : api_{api}
  {}

  Unknown_api(Unknown_api&& rhs) noexcept
    : api_{rhs.api_}
  {
    rhs.api_ = nullptr;
  }

  Unknown_api(const Unknown_api& rhs) noexcept
    : api_{rhs.api_}
  {
    if (api_)
      api_->AddRef();
  }

  Unknown_api& operator=(Unknown_api&& rhs) noexcept
  {
    Unknown_api tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  Unknown_api& operator=(const Unknown_api& rhs) noexcept
  {
    Unknown_api tmp{rhs};
    swap(tmp);
    return *this;
  }

  void swap(Unknown_api& rhs) noexcept
  {
    using std::swap;
    swap(api_, rhs.api_);
  }

  template<class T = Api>
  const T& api() const
  {
    check(api_, "invalid "+std::string{typeid(Derived).name()}+" instance used");
    return query_interface<T>(const_cast<Api&>(*api_));
  }

  template<class T = Api>
  T& api()
  {
    return const_cast<T&>(static_cast<const Unknown_api*>(this)->api<T>());
  }

  explicit operator bool() const noexcept
  {
    return static_cast<bool>(api_);
  }

private:
  template<class> friend class Ptr;

  Api* api_{};
};

// -----------------------------------------------------------------------------
// Ptr
// -----------------------------------------------------------------------------

template<class A>
class Ptr final : public Unknown_api<Ptr<A>, A> {
  using Ua = Unknown_api<Ptr, A>;
public:
  using Ua::Ua;

  A* get() const noexcept
  {
    return Ua::api_;
  }

  A* operator->() const noexcept
  {
    return get();
  }

  template<class T>
  Ptr<T> to() const
  {
    return Ptr<T>::query(Ua::api_);
  }
};

// -----------------------------------------------------------------------------
// Basic_com_object
// -----------------------------------------------------------------------------

template<class Object, class ObjectInterface>
class Basic_com_object : private Noncopy {
public:
  using Api = ObjectInterface;
  using Instance = Object;

  virtual ~Basic_com_object()
  {
    if (api_) {
      api_->Release();
      api_ = nullptr;
    }
  }

  Basic_com_object()
    : Basic_com_object{CLSCTX_INPROC_SERVER, nullptr}
  {}

  // The instance takes ownership of `api`, so `api->AddRef()` not called.
  explicit Basic_com_object(ObjectInterface* const api)
    : api_{api}
  {}

  explicit Basic_com_object(const DWORD context_mask,
    IUnknown* const aggregate = {})
  {
    if (const auto err = CoCreateInstance(
        __uuidof(Object),
        aggregate,
        context_mask,
        __uuidof(ObjectInterface),
        reinterpret_cast<LPVOID*>(&api_)); err != S_OK)
      throw Win_error{"cannot create COM object", err};
    if (!api_)
      throw std::logic_error{"invalid COM instance created"};
  }

  Basic_com_object(Basic_com_object&& rhs) noexcept
    : api_{rhs.api_}
  {
    rhs.api_ = nullptr;
  }

  Basic_com_object& operator=(Basic_com_object&& rhs) noexcept
  {
    Basic_com_object tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Basic_com_object& rhs) noexcept
  {
    using std::swap;
    swap(api_, rhs.api_);
  }

  template<class T = Api>
  const T& api() const
  {
    check(api_, "invalid "
      +std::string{typeid(Basic_com_object).name()}+" instance used");
    return query_interface<T>(const_cast<ObjectInterface&>(*api_));
  }

  template<class T = Api>
  T& api()
  {
    return const_cast<T&>(static_cast<const Basic_com_object*>(this)->api<T>());
  }

  explicit operator bool() const noexcept
  {
    return static_cast<bool>(api_);
  }

private:
  ObjectInterface* api_{};
};

// -----------------------------------------------------------------------------
// Advise_sink
// -----------------------------------------------------------------------------

template<class ComInterface>
class Advise_sink : public ComInterface, private Noncopymove {
  static_assert(std::is_base_of_v<IDispatch, ComInterface>);
public:
  virtual void set_owner(void* owner) = 0;

  REFIID interface_id() const noexcept
  {
    return __uuidof(ComInterface);
  }

  // IUnknown overrides

  HRESULT QueryInterface(REFIID id, void** const object) override
  {
    if (!object)
      return E_POINTER;

    if (id == __uuidof(ComInterface))
      *object = static_cast<ComInterface*>(this);
    else if (id == __uuidof(IDispatch))
      *object = static_cast<IDispatch*>(this);
    else if (id == __uuidof(IUnknown))
      *object = static_cast<IUnknown*>(this);
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

  // IDispatch overrides

  HRESULT GetTypeInfoCount(UINT* const info) override
  {
    *info = 0;
    return S_OK;
  }

  HRESULT GetTypeInfo(const UINT info, const LCID cid,
    ITypeInfo** const tinfo) override
  {
    if (info)
      return DISP_E_BADINDEX;

    *tinfo = nullptr;
    return S_OK;
  }

  HRESULT GetIDsOfNames(REFIID riid, LPOLESTR* const names,
    const UINT name_count, const LCID cid, DISPID* const disp_id) override
  {
    if (!disp_id)
      return E_OUTOFMEMORY;

    for (UINT i{}; i < name_count; ++i)
      disp_id[i] = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
  }

private:
  ULONG ref_count_{};
};

// -----------------------------------------------------------------------------
// Advise_sink_connection
// -----------------------------------------------------------------------------

template<class AdviseSink>
class Advise_sink_connection : private Noncopymove {
public:
  virtual ~Advise_sink_connection()
  {
    if (point_) {
      point_->Unadvise(sink_connection_token_);
      point_->Release();
      point_ = nullptr;
    }

    if (point_container_) {
      point_container_->Release();
      point_container_ = nullptr;
    }
  }

  Advise_sink_connection(IUnknown& com, std::unique_ptr<AdviseSink> sink,
    void* const sink_owner)
    : sink_{std::move(sink)}
  {
    const char* errmsg{};
    if (!sink_)
      throw std::invalid_argument{"invalid AdviseSink instance"};
    else if (com.QueryInterface(&point_container_) != S_OK)
      errmsg = "cannot query interface of COM object";
    else if (point_container_->FindConnectionPoint(sink_->interface_id(),
        &point_) != S_OK)
      errmsg = "cannot find sink connection point of COM object";
    else if (point_->Advise(sink_.get(), &sink_connection_token_) != S_OK)
      errmsg = "cannot get sink connection token";

    if (errmsg)
      throw std::runtime_error{errmsg};

    sink_->set_owner(sink_owner);
  }

  const AdviseSink& sink() const noexcept
  {
    return *sink_;
  }

  AdviseSink& sink() noexcept
  {
    return const_cast<AdviseSink&>(
      static_cast<const Advise_sink_connection*>(this)->sink());
  }

private:
  std::unique_ptr<AdviseSink> sink_;
  DWORD sink_connection_token_{};
  IConnectionPointContainer* point_container_{};
  IConnectionPoint* point_{};
};

// -----------------------------------------------------------------------------
// Standard_marshaler
// -----------------------------------------------------------------------------

class Standard_marshaler final : public
  Unknown_api<Standard_marshaler, IMarshal> {
  using Ua = Unknown_api<Standard_marshaler, IMarshal>;
public:
  Standard_marshaler(REFIID riid, const LPUNKNOWN unknown,
    const MSHCTX dest_ctx, const MSHLFLAGS flags)
  {
    IMarshal* instance{};
    const auto err = CoGetStandardMarshal(riid, unknown, dest_ctx, nullptr,
      flags, &instance);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get standard marshaler");
    Ua tmp{instance};
    swap(tmp);
  }
};

// -----------------------------------------------------------------------------

namespace detail {

template<class Api, class ComObject>
[[nodiscard]] auto& api(const ComObject& com)
{
  using Com = std::decay_t<decltype(com)>;
  return const_cast<Com&>(com).api<Api>();
}

template<class ComObject>
[[nodiscard]] auto& api(const ComObject& com)
{
  return api<typename ComObject::Api>(com);
}

inline auto* c_str(const std::string& s) noexcept
{
  return s.c_str();
}

inline auto* c_str(const std::wstring& s) noexcept
{
  return s.c_str();
}

inline auto* c_str(const char*& s) noexcept
{
  return s;
}

inline auto* c_str(const wchar_t*& s) noexcept
{
  return s;
}

template<typename String>
inline _bstr_t bstr(const String& s)
{
  return _bstr_t{c_str(s)};
}

inline VARIANT_BOOL variant_bool(const bool value) noexcept
{
  return value ? VARIANT_TRUE : VARIANT_FALSE;
}

template<class Wrapper, class Api, typename T, typename U>
void set(const char* const what,
  const Wrapper& wrapper, HRESULT(Api::* setter)(T),
  const U value)
{
  DMITIGR_ASSERT(what);
  DMITIGR_ASSERT(setter);
  const auto err = (api<Api>(wrapper).*setter)(value);
  DMITIGR_WINCOM_THROW_IF_ERROR(err, std::string{"cannot set "}.append(what));
}

template<class Wrapper, class Api>
void set(const char* const what,
  const Wrapper& wrapper, HRESULT(Api::* setter)(VARIANT_BOOL),
  const bool value)
{
  set(what, wrapper, setter, variant_bool(value));
}

template<class Wrapper, class Api, class T>
void set(const char* const what,
  const Wrapper& wrapper, HRESULT(Api::* setter)(BSTR),
  const T& value)
{
  using std::is_same_v;
  if constexpr (!is_same_v<T, BSTR> && !is_same_v<T, _bstr_t>)
    set(what, wrapper, setter, bstr(value));
  else
    set(what, wrapper, setter, value);
}

template<class Wrapper, class Api, typename T>
T get(const char* const what,
  const Wrapper& wrapper, HRESULT(Api::* getter)(T*))
{
  DMITIGR_ASSERT(what);
  DMITIGR_ASSERT(getter);
  T result{};
  const auto err = (api<Api>(wrapper).*getter)(&result);
  DMITIGR_WINCOM_THROW_IF_ERROR(err, std::string{"cannot get "}.append(what));
  return result;
}

template<class Wrapper, class Api>
bool get_bool(const char* const what,
  const Wrapper& wrapper, HRESULT(Api::* getter)(VARIANT_BOOL*))
{
  return get(what, wrapper, getter) == VARIANT_TRUE;
}

template<class String, class Wrapper, class Api>
String get(const Wrapper& wrapper, HRESULT(Api::* getter)(BSTR*))
{
  DMITIGR_ASSERT(getter);
  BSTR value{};
  (detail::api<Api>(wrapper).*getter)(&value);
  if (!value) {
    if constexpr (Is_optional_v<String>)
      return String{};
    else
      throw std::runtime_error{"cannot get BSTR value"};
  }
  _bstr_t tmp{value, false}; // take ownership
  return String(tmp);
}

} // namespace detail

} // namespace dmitigr::wincom
