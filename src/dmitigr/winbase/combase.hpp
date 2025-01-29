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

#pragma once
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")

#include "../base/noncopymove.hpp"
#include "../base/traits.hpp"
#include "strconv.hpp"

#include <algorithm>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <Objbase.h>
#include <oaidl.h> // VARIANT
#include <oleauto.h> // VARIANT manipulators

namespace dmitigr::winbase::com {

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------

template<bool IsConst, bool IsOwns>
class Basic_safe_array;

using Safe_array = Basic_safe_array<false, true>;
using Const_safe_array = Basic_safe_array<true, true>;

using Safe_array_view = Basic_safe_array<false, false>;
using Const_safe_array_view = Basic_safe_array<true, false>;

// -----------------------------------------------------------------------------

template<bool IsConst, bool IsOwns>
class Basic_variant;

using Variant = Basic_variant<false, true>;
using Const_variant = Basic_variant<true, true>;

using Variant_view = Basic_variant<false, false>;
using Const_variant_view = Basic_variant<true, false>;

// -----------------------------------------------------------------------------
// Memory
// -----------------------------------------------------------------------------

template<typename T>
class Taskmem final : private Noncopymove {
public:
  Taskmem() = default;

  ~Taskmem()
  {
    CoTaskMemFree(value_);
  }

  explicit Taskmem(T* const value)
    : value_{value}
  {}

  T* value() noexcept
  {
    return value_;
  }

  const T* value() const noexcept
  {
    return value_;
  }

  bool is_valid() const noexcept
  {
    return static_cast<bool>(value_);
  }

  explicit operator bool() const noexcept
  {
    return is_valid();
  }

private:
  T* value_{};
};

inline auto to_com_string(REFCLSID id)
{
  LPOLESTR str{};
  const HRESULT err{StringFromCLSID(id, &str)};
  if (err == E_OUTOFMEMORY)
    throw std::bad_alloc{};
  else if (!SUCCEEDED(err))
    throw std::runtime_error{"cannot convert CLSID to string: error "
      +std::to_string(err)};
  return Taskmem<OLECHAR>{str};
}

// -----------------------------------------------------------------------------
// Registry
// -----------------------------------------------------------------------------

inline std::wstring server_registry_root(REFCLSID id)
{
  return std::wstring{LR"(SOFTWARE\Classes\CLSID\)"}
    .append(to_com_string(id).value());
}

inline std::wstring server_registry_localserver32(REFCLSID id)
{
  return server_registry_root(id).append(LR"(\LocalServer32)");
}

// -----------------------------------------------------------------------------
// BSTR
// -----------------------------------------------------------------------------

inline std::wstring_view to_wstring_view(const BSTR bstr)
{
  return {bstr, SysStringLen(bstr)};
}

inline std::wstring to_wstring(const BSTR bstr)
{
  return std::wstring{to_wstring_view(bstr)};
}

inline std::string to_string(const BSTR bstr, const UINT code_page = CP_UTF8)
{
  return winbase::utf16_to_utf8(to_wstring_view(bstr), code_page);
}

// -----------------------------------------------------------------------------
// VARIANT
// -----------------------------------------------------------------------------

template<bool IsConst, bool IsOwns>
class Basic_variant final {
public:
  static constexpr const bool is_const{IsConst};
  static constexpr const bool is_owns{IsOwns};

  ~Basic_variant()
  {
    if constexpr (IsOwns)
      VariantClear(&data_);
  }

  Basic_variant()
  {
    VariantInit(&data_);
  }

  Basic_variant(const VARIANT dat)
  {
    VariantInit(&data_);
    data_ = dat;
  }

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns && !IsRhsOwns>>
  explicit Basic_variant(const Basic_variant<IsRhsConst, !IsRhsOwns>& rhs)
  {
    copy_from(rhs);
  }

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns || !IsOwns && IsRhsOwns>>
  Basic_variant(const Basic_variant<IsRhsConst, IsRhsOwns>& rhs)
  {
    static_assert(IsOwns || IsConst || !IsRhsConst);
    VariantInit(&data_);
    if constexpr (IsOwns)
      copy_from(rhs);
    else
      data_ = rhs.data_;
  }

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns || !IsOwns && IsRhsOwns>>
  Basic_variant& operator=(const Basic_variant<IsRhsConst, IsRhsOwns>& rhs)
  {
    Basic_variant tmp{rhs};
    swap(tmp);
    return *this;
  }

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns>>
  Basic_variant(Basic_variant<IsRhsConst, IsRhsOwns>&& rhs) noexcept
  {
    static_assert(IsConst || !IsRhsConst);
    VariantInit(&data_);
    data_ = rhs.data_;
    VariantInit(&rhs.data_);
  }

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns>>
  Basic_variant& operator=(Basic_variant<IsRhsConst, IsRhsOwns>&& rhs) noexcept
  {
    Basic_variant tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Basic_variant& rhs) noexcept
  {
    using std::swap;
    swap(data_, rhs.data_);
  }

  VARENUM type() const noexcept
  {
    return static_cast<VARENUM>(data_.vt);
  }

  Variant to_variant(const VARENUM vt, const USHORT flags = {}) const
  {
    Variant result;
    if (VariantChangeType(&result.data_, &data_, flags, vt) != S_OK)
      throw std::runtime_error{"cannot convert VARIANT to VARIANT"};
    return result;
  }

  bool is_empty() const noexcept
  {
    return is(VT_EMPTY);
  }

  bool is_null() const noexcept
  {
    return is(VT_NULL);
  }

  const BSTR as_bstr() const
  {
    check(VT_BSTR, "BSTR string");
    return data_.bstrVal;
  }

  std::string as_string_utf8() const
  {
    check(VT_BSTR, "UTF-8 string");
    return com::to_string(data_.bstrVal);
  }

  std::string as_string_acp() const
  {
    check(VT_BSTR, "ACP string");
    return com::to_string(data_.bstrVal, CP_ACP);
  }

  std::wstring as_wstring() const
  {
    check(VT_BSTR, "UTF-16 string");
    return com::to_wstring(data_.bstrVal);
  }

  std::int8_t as_int8() const
  {
    check(VT_I1, "int8");
    return data_.cVal;
  }

  std::uint8_t as_uint8() const
  {
    check(VT_UI1, "uint8");
    return data_.bVal;
  }

  std::int16_t as_int16() const
  {
    check(VT_I2, "int16");
    return data_.iVal;
  }

  std::uint16_t as_uint16() const
  {
    check(VT_UI2, "uint16");
    return data_.uiVal;
  }

  std::int32_t as_int32() const
  {
    if (is(VT_I4))
      return data_.lVal;
    else if (is(VT_INT))
      return data_.intVal;
    throw_conversion_error("int32");
  }

  std::uint32_t as_uint32() const
  {
    if (is(VT_UI4))
      return data_.ulVal;
    else if (is(VT_UINT))
      return data_.uintVal;
    throw_conversion_error("uint32");
  }

  std::int64_t as_int64() const
  {
    check(VT_I8, "int64");
    return data_.llVal;
  }

  std::uint64_t as_uint64() const
  {
    check(VT_UI8, "uint64");
    return data_.ullVal;
  }

  float as_real32() const
  {
    check(VT_R4, "real32");
    return data_.fltVal;
  }

  double as_real64() const
  {
    check(VT_R8, "real64");
    return data_.dblVal;
  }

  bool as_bool() const
  {
    check(VT_BOOL, "bool");
    return data_.boolVal == VARIANT_TRUE;
  }

  DATE as_date() const
  {
    check(VT_DATE, "DATE");
    return data_.date;
  }

  const PVOID as_pvoid() const
  {
    check_bits(VT_BYREF, "PVOID");
    return data_.byref;
  }

  PVOID as_pvoid()
  {
    static_assert(!IsConst);
    check_bits(VT_BYREF, "PVOID");
    return data_.byref;
  }

  Const_safe_array_view as_array() const;

  Safe_array_view as_array();

  const VARIANT& data() const noexcept
  {
    return data_;
  }

  VARIANT& data() noexcept
  {
    static_assert(!IsConst);
    return data_;
  }

private:
  mutable VARIANT data_{};

  template<bool, bool> friend class Basic_variant;

  void copy_from(const Basic_variant& rhs)
  {
    const auto err = VariantCopyInd(&data_, &rhs.data_);
    if (FAILED(err))
      // FIXME: use wincom::Win_error
      throw std::runtime_error{"cannot copy Variant"};
  }

  bool is(const VARENUM tp) const noexcept
  {
    return data_.vt == tp;
  }

  bool has(const VARENUM tp) const noexcept
  {
    return bool(data_.vt & tp);
  }

  [[noreturn]] static void throw_conversion_error(const std::string& tpnm)
  {
    throw std::logic_error{"cannot convert Variant to "+tpnm};
  }

  void check(const VARENUM tp, const std::string& tpnm) const
  {
    if (!is(tp))
      throw_conversion_error(tpnm);
  }

  void check_bits(const VARENUM tp, const std::string& tpnm) const
  {
    if (!has(tp))
      throw_conversion_error(tpnm);
  }
};

struct Date final {
  DATE value{};
};
static_assert(sizeof(Date) == sizeof(DATE));

namespace detail {
template<typename> struct Variant_type_traits final {
  static_assert(false, "not specialized");
};
template<> struct Variant_type_traits<std::int8_t> final {
  static constexpr const VARENUM vt{VT_I1};
};
template<> struct Variant_type_traits<std::uint8_t> final {
  static constexpr const VARENUM vt{VT_UI1};
};
template<> struct Variant_type_traits<std::int16_t> final {
  static constexpr const VARENUM vt{VT_I2};
};
template<> struct Variant_type_traits<std::uint16_t> final {
  static constexpr const VARENUM vt{VT_UI2};
};
template<> struct Variant_type_traits<std::int32_t> final {
  static constexpr const VARENUM vt{VT_I4};
};
template<> struct Variant_type_traits<std::uint32_t> final {
  static constexpr const VARENUM vt{VT_UI4};
};
template<> struct Variant_type_traits<std::int64_t> final {
  static constexpr const VARENUM vt{VT_I8};
};
template<> struct Variant_type_traits<std::uint64_t> final {
  static constexpr const VARENUM vt{VT_UI8};
};
template<> struct Variant_type_traits<float> final {
  static constexpr const VARENUM vt{VT_R4};
};
template<> struct Variant_type_traits<double> final {
  static constexpr const VARENUM vt{VT_R8};
};
template<> struct Variant_type_traits<bool> final {
  static constexpr const VARENUM vt{VT_BOOL};
};
template<> struct Variant_type_traits<Date> final {
  static constexpr const VARENUM vt{VT_DATE};
};
template<> struct Variant_type_traits<PVOID> final {
  static constexpr const VARENUM vt{VT_BYREF};
};
template<typename Ch, class Tr, class Al>
struct Variant_type_traits<std::basic_string<Ch, Tr, Al>> final {
  static constexpr const VARENUM vt{VT_BSTR};
};
} // namespace detail

/**
 * @param flags Flags which are passed to Basic_variant::to_variant().
 *
 * @returns The value of type `T` coerced from `variant`.
 */
template<typename T, bool IsConst, bool IsOwns>
T to(const Basic_variant<IsConst, IsOwns>& variant, const USHORT flags = {})
{
  using D = std::decay_t<T>;
  using std::is_same_v;
  const auto var = variant.to_variant(detail::Variant_type_traits<T>::vt, flags);
  if constexpr (is_same_v<D, bool>)
    return var.as_bool();
  else if constexpr (is_same_v<D, Date>)
    return Date{var.as_date()};
  else if constexpr (is_same_v<D, std::int8_t>)
    return var.as_int8();
  else if constexpr (is_same_v<D, std::uint8_t>)
    return var.as_uint8();
  else if constexpr (is_same_v<D, std::int16_t>)
    return var.as_int16();
  else if constexpr (is_same_v<D, std::uint16_t>)
    return var.as_uint16();
  else if constexpr (is_same_v<D, std::int32_t>)
    return var.as_int32();
  else if constexpr (is_same_v<D, std::uint32_t>)
    return var.as_uint32();
  else if constexpr (is_same_v<D, std::int64_t>)
    return var.as_int64();
  else if constexpr (is_same_v<D, std::uint64_t>)
    return var.as_uint64();
  else if constexpr (is_same_v<D, float>)
    return var.as_real32();
  else if constexpr (is_same_v<D, double>)
    return var.as_real64();
  else if constexpr (is_same_v<D, std::string>)
    return var.as_string_utf8();
  else if constexpr (is_same_v<D, std::wstring>)
    return var.as_wstring();
  else
    static_assert(false_value<T>);
}

// -----------------------------------------------------------------------------
// SAFEARRAY
// -----------------------------------------------------------------------------

/// A wrapper around SAFEARRAY.
template<bool IsConst, bool IsOwns>
class Basic_safe_array final {
public:
  static constexpr const bool is_const{IsConst};
  static constexpr const bool is_owns{IsOwns};

  /// Destroys the underlying data if `IsOwns`.
  ~Basic_safe_array()
  {
    if (IsOwns)
      SafeArrayDestroy(data_);
  }

  /// Constructs an empty instance.
  Basic_safe_array() = default;

  /**
   * @brief Constructs an array of elements of the specified type.
   *
   * @param vt The array elements type.
   * @param rgsa The bounds of each dimension of the array.
   */
  Basic_safe_array(const VARTYPE vt, std::vector<SAFEARRAYBOUND> rgsa)
    : data_{SafeArrayCreate(vt, rgsa.size(), rgsa.data())}
  {
    if (!data_)
      throw std::runtime_error{"cannot create Safe_array"};
  }

  /**
   * @brief Constructs an array.
   *
   * @param data The existing array.
   */
  explicit Basic_safe_array(SAFEARRAY* const data)
    : data_{data}
  {}

  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns && !IsRhsOwns>>
  explicit Basic_safe_array(const Basic_safe_array<IsRhsConst, !IsRhsOwns>& rhs)
  {
    copy_from(rhs);
  }

  /// Copies the instance.
  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns || !IsOwns && IsRhsOwns>>
  Basic_safe_array(const Basic_safe_array& rhs)
  {
    static_assert(IsOwns || IsConst || !IsRhsConst);
    if constexpr (IsOwns)
      copy_from(rhs);
    else
      data_ = rhs.data_;
  }

  /// Copy-assignable.
  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns || !IsOwns && IsRhsOwns>>
  Basic_safe_array& operator=(const Basic_safe_array<IsRhsConst, IsRhsOwns>& rhs)
  {
    Basic_safe_array tmp{rhs};
    swap(tmp);
    return *this;
  }

  /// Move-constructible.
  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns>>
  Basic_safe_array(Basic_safe_array&& rhs) noexcept
    : data_{rhs.data_}
  {
    static_assert(IsConst || !IsRhsConst);
    rhs.data_ = {};
  }

  /// Move-assignable.
  template<bool IsRhsConst, bool IsRhsOwns,
    typename = std::enable_if_t<IsOwns == IsRhsOwns>>
  Basic_safe_array& operator=(Basic_safe_array&& rhs) noexcept
  {
    Basic_safe_array tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  /// Swaps this instance with `rhs`.
  void swap(Basic_safe_array& rhs) noexcept
  {
    using std::swap;
    swap(data_, rhs.data_);
  }

  // Forward declaration of an array slice.
  template<bool IsConst> class Basic_slice;

  /// A modifiable slice.
  using Slice = Basic_slice<false>;

  /// A const slice.
  using Const_slice = Basic_slice<true>;

  /// An array slice.
  template<bool IsConst>
  class Basic_slice final : Noncopymove {
  public:
    /// Decrements the lock count of the array.
    ~Basic_slice()
    {
      if (self_.data_)
        SafeArrayUnlock(self_.data_);
    }

    /**
     * @tparam Can be `BSTR`, `IUnknown`, `IDispatch` and `VARIANT`.
     *
     * @returns The pointer to the first element of the underlying part of array
     * which is represented by this slice.
     */
    template<typename T>
    const T* array() const
    {
      using std::is_same_v;
      using D = std::decay_t<T>;
      USHORT feat{};
      const char* msg{};
      if constexpr (is_same_v<D, BSTR>)
        feat = FADF_BSTR;
      else if constexpr (is_same_v<D, IUnknown>)
        feat = FADF_UNKNOWN;
      else if constexpr (is_same_v<D, IDispatch>)
        feat = FADF_DISPATCH;
      else if constexpr (is_same_v<D, VARIANT>)
        feat = FADF_VARIANT | FADF_HAVEVARTYPE;
      else
        static_assert(false_value<T>);
      if (!bool(self_.features() & feat))
        throw std::runtime_error{"cannot get array of requested type"};
      return static_cast<const T*>(self_.data().pvData) + absolute_offset_;
    }

    /// @overload
    template<typename T>
    T* array()
    {
      static_assert(!IsConst);
      return const_cast<T*>(static_cast<const Basic_slice*>(this)->array<T>());
    }

    /// @returns An instance of Variant at the specified `index`.
    Const_variant_view variant(const std::size_t index) const
    {
      return Const_variant_view{array<VARIANT>()[index]};
    }

    /// @overload
    Variant_view variant(const std::size_t index)
    {
      static_assert(!IsConst);
      return Variant_view{array<VARIANT>()[index]};
    }

    /// @returns The dimension of this slice.
    USHORT dimension() const noexcept
    {
      return dim_;
    }

    /// @returns The size of this slice.
    std::size_t size() const noexcept
    {
      return size_;
    }

    /// @returns The lower bound of this slice.
    LONG lower_bound() const
    {
      return self_.data().rgsabound[dim_].lLbound;
    }

    /// @returns `true` if this slice represents a vector.
    bool is_vector() const
    {
      return dimension() == self_.dimension_count() - 1;
    }

    /// @returns The slice count.
    std::size_t slice_count() const
    {
      return !is_vector() * self_.data().rgsabound[dim_].cElements;
    }

    /**
     * @param index Zero-based slice index.
     *
     * @returns The specified slice.
     */
    Const_slice slice(const std::size_t index) const
    {
      return make_slice<Const_slice>(index);
    }

    /// @overload
    Slice slice(const std::size_t index)
    {
      static_assert(!IsConst);
      return make_slice<Slice>(index);
    }

  private:
    friend Basic_safe_array;

    const Basic_safe_array& self_;
    USHORT dim_{};
    std::size_t absolute_offset_{};
    std::size_t size_{};

    template<class S>
    S make_slice(const std::size_t index) const
    {
      const auto& data = self_.data();
      if (!(dim_ + 1 < data.cDims))
        throw std::invalid_argument{"Safe_array dimension overflow"};
      else if (!(index < data.rgsabound[dim_].cElements))
        throw std::invalid_argument{"Safe_array index overflow"};
      return S{self_, static_cast<USHORT>(dim_ + 1), index, absolute_offset_};
    }

    /// Increments the lock count of the array.
    Basic_slice(const Basic_safe_array& self, const USHORT dim,
      const std::size_t slice_offset,
      const std::size_t absolute_offset)
      : self_{self}
      , dim_{dim}
    {
      {
        const auto err = SafeArrayLock(self.data_);
        if (FAILED(err))
          throw std::runtime_error{"cannot create Safe_array::Slice:"
            " cannot lock SAFEARRAY"};
      }

      size_ = [&]
      {
        std::size_t result{1};
        const USHORT dcount{self_.dimension_count()};
        for (USHORT d{dim_}; d < dcount; ++d)
          result *= self_.data().rgsabound[d].cElements;
        return result;
      }();
      absolute_offset_ = absolute_offset + slice_offset*size_;
    }
  };

  /// @returns The VARTYPE stored in the underlying safe array.
  std::optional<VARTYPE> vartype() const
  {
    if (bool(features() & FADF_HAVEVARTYPE)) {
      VARTYPE result{};
      if (FAILED(SafeArrayGetVartype(data_, &result)))
        throw std::runtime_error{"cannot get VARTYPE of SAFEARRAY"};
      return result;
    } else
      return std::nullopt;
  }

  /// @returns The dimension count.
  USHORT dimension_count() const
  {
    return data().cDims;
  }

  /// @returns The feature flags.
  USHORT features() const
  {
    return data().fFeatures;
  }

  /// @returns The element size.
  ULONG element_size() const
  {
    return data().cbElements;
  }

  /// @returns The value of lock counter of the underlying data.
  ULONG lock_count() const
  {
    return data().cLocks;
  }

  /**
   * @brief Increments the lock count of the underying array.
   *
   * @returns The slice of zero dimension.
   */
  Const_slice slice() const
  {
    return Const_slice{*this, 0, 0, 0};
  }

  /// @overload
  Slice slice()
  {
    static_assert(!IsConst);
    return Slice{*this, 0, 0, 0};
  }

  /// @returns The underlying data.
  const SAFEARRAY& data() const
  {
    if (!data_)
      throw std::logic_error{"cannot use invalid instance of Safe_array"};
    return *data_;
  }

  /// @overload
  SAFEARRAY& data()
  {
    static_assert(!IsConst);
    return const_cast<SAFEARRAY&>(static_cast<const Basic_safe_array*>(this)->data());
  }

  /// @returns The underlying data.
  const SAFEARRAY* data_ptr() const noexcept
  {
    return data_;
  }

  /// @overload
  SAFEARRAY* data_ptr() noexcept
  {
    static_assert(!IsConst);
    return data_;
  }

private:
  SAFEARRAY* data_{};

  void copy_from(Basic_safe_array& rhs)
  {
    if (rhs.data_) {
      const auto err = SafeArrayCopy(rhs.data_, &data_);
      if (FAILED(err))
        // FIXME: use wincom::Win_error
        throw std::runtime_error{"cannot copy Safe_array"};
    }
  }
};

// -----------------------------------------------------------------------------
// VARIANT (implementations)
// -----------------------------------------------------------------------------

template<bool IsConst, bool IsOwns>
Const_safe_array_view Basic_variant<IsConst, IsOwns>::as_array() const
{
  check_bits(static_cast<VARENUM>(VT_VECTOR|VT_ARRAY), "SAFEARRAY");
  return Const_safe_array_view{data_.parray};
}

template<bool IsConst, bool IsOwns>
Safe_array_view Basic_variant<IsConst, IsOwns>::as_array()
{
  static_assert(!IsConst);
  check_bits(static_cast<VARENUM>(VT_VECTOR|VT_ARRAY), "SAFEARRAY");
  return Safe_array_view{data_.parray};
}

} // namespace dmitigr::winbase::com
