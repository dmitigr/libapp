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
#pragma comment(lib, "advapi32")

#include "../base/assert.hpp"
#include "../base/noncopymove.hpp"
#include "../base/traits.hpp"
#include "exceptions.hpp"

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace dmitigr::winbase::registry {

/// A very thin wrapper around the HKEY data type.
class Hkey_guard final : private Noncopy {
public:
  /// The destructor.
  ~Hkey_guard()
  {
    close();
  }

  /// The constructor.
  explicit Hkey_guard(const HKEY handle = NULL) noexcept
    : handle_{handle}
  {}

  /// The move constructor.
  Hkey_guard(Hkey_guard&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = NULL;
  }

  /// The move assignment operator.
  Hkey_guard& operator=(Hkey_guard&& rhs) noexcept
  {
    if (this != &rhs) {
      Hkey_guard tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// The swap operation.
  void swap(Hkey_guard& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  /// @returns The guarded HKEY.
  HKEY handle() const noexcept
  {
    return handle_;
  }

  /// @returns The guarded HKEY.
  operator HKEY() const noexcept
  {
    return handle();
  }

  /// @returns The error code.
  LSTATUS close() noexcept
  {
    LSTATUS result{ERROR_SUCCESS};
    if (handle_ != NULL) {
      if ( (result = RegCloseKey(handle_)) != ERROR_SUCCESS)
        handle_ = NULL;
    }
    return result;
  }

private:
  HKEY handle_{NULL};
};

inline Hkey_guard open_key(const HKEY key, LPCWSTR const subkey, const REGSAM mask,
  const DWORD options = 0)
{
  HKEY result{};
  const auto err = RegOpenKeyExW(key, subkey, options, mask, &result);
  if (err == ERROR_FILE_NOT_FOUND)
    return Hkey_guard{};
  else if (err != ERROR_SUCCESS)
    throw Sys_exception{static_cast<DWORD>(err), "cannot open registry key"};
  return Hkey_guard{result};
}

/// @returns A pair with created/opened key and disposition information.
inline std::pair<Hkey_guard, DWORD>
create_key(const HKEY key, LPCWSTR const subkey,
  const REGSAM mask, const LPSECURITY_ATTRIBUTES secattrs = {},
  const DWORD options = 0)
{
  HKEY res_key{};
  DWORD res_disp{};
  const auto err = RegCreateKeyExW(key,
    subkey,
    0,
    NULL,
    options,
    mask,
    secattrs,
    &res_key,
    &res_disp);
  if (err != ERROR_SUCCESS)
    throw Sys_exception{static_cast<DWORD>(err), "cannot create registry key"};
  return std::make_pair(Hkey_guard{res_key}, res_disp);
}

/// @overload
inline auto create_key(const HKEY key, const std::wstring& subkey,
  const REGSAM mask, const LPSECURITY_ATTRIBUTES secattrs = {},
  const DWORD options = 0)
{
  return create_key(key, subkey.c_str(), mask, secattrs, options);
}

inline void set_value(const HKEY key, LPCWSTR const name, const DWORD type,
  const BYTE* const data, const DWORD size)
{
  const auto err = RegSetValueExW(key, name, 0, type, data, size);
  if (err != ERROR_SUCCESS)
    throw Sys_exception{static_cast<DWORD>(err), "cannot set value of registry key"};
}

template<typename T>
void set_value(const HKEY key, LPCWSTR const name, const T& value)
{
  if constexpr (std::is_same_v<T, DWORD>) {
    set_value(key, name, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(T));
  } else if constexpr (std::is_same_v<T, LPCWSTR>) {
    const auto* const bytes = reinterpret_cast<const BYTE*>(value);
    set_value(key, name, REG_SZ, bytes, sizeof(*value)*(lstrlenW(value) + 1));
  } else if constexpr (std::is_same_v<T, std::wstring>) {
    const auto* const bytes = reinterpret_cast<const BYTE*>(value.c_str());
    set_value(key, name, REG_SZ, bytes,
      sizeof(std::wstring::value_type)*(value.size() + 1));
  } else
    static_assert(false_value<T>, "unsupported type specified");
}

/// @overload
template<typename T>
void set_value(const HKEY key, const std::wstring& name, const T& value)
{
  set_value(key, name.c_str(), value);
}

inline void remove_value(const HKEY key,
  LPCWSTR const subkey = {}, LPCWSTR const value_name = {})
{
  const auto err = RegDeleteKeyValueW(key, subkey, value_name);
  if (err != ERROR_FILE_NOT_FOUND && err != ERROR_SUCCESS)
    throw Sys_exception{static_cast<DWORD>(err),
      "cannot remove value of registry key"};
}

/// @overload
inline void remove_value(const HKEY key, const std::wstring& subkey = {},
  const std::wstring& value_name = {})
{
  LPCWSTR subkey_str{!subkey.empty() ? subkey.c_str() : nullptr};
  LPCWSTR value_name_str{!value_name.empty() ? value_name.c_str() : nullptr};
  remove_value(key, subkey_str, value_name_str);
}

inline void remove_key(const HKEY key, LPCWSTR subkey)
{
  if (const auto err = RegDeleteKeyW(key, subkey); err != ERROR_SUCCESS)
    throw Sys_exception{static_cast<DWORD>(err),
      "cannot remove registry key"};
}

/// @overload
inline void remove_key(const HKEY key, const std::wstring& subkey)
{
  remove_key(key, subkey.c_str());
}

/**
 * @returns The number of bytes, including `2` extra terminating nulls,
 * required to store the requested string, or `0` if no such a string found.
 */
inline DWORD required_buffer_size(const HKEY key,
  LPCWSTR const subkey, LPCWSTR const name)
{
  DWORD result{};
  const auto err = RegGetValueW(key,
    subkey,
    name,
    RRF_RT_REG_SZ,
    NULL,
    NULL,
    &result);
  if (err == ERROR_FILE_NOT_FOUND)
    return 0;
  else if (err != ERROR_SUCCESS || !result)
    throw std::logic_error{"API bug"};
  return result;
}

/**
 * @tparam T The result type.
 *
 * @returns The string stored in the registry.
 *
 * @throws `std::runtime_error` if `T` is `std::wstring` and
 * `required_buffer_size(key, subkey, name) % sizeof(T::value_type) != 0`.
 */
template<typename T>
std::optional<T> value(const HKEY key, LPCWSTR const subkey, LPCWSTR const name)
{
  static const auto result_or_throw = [](auto&& result, const auto error)
    -> std::optional<T>
  {
    if (error == ERROR_FILE_NOT_FOUND)
      return std::nullopt;
    else if (error == ERROR_SUCCESS)
      return std::make_optional<T>(std::move(result));
    else
      throw Sys_exception{static_cast<DWORD>(error),
        "cannot get value of registry key"};
  };

  using Type = std::decay_t<T>;
  using std::is_same_v;
  if constexpr (is_same_v<Type, DWORD>) {
    DWORD result{};
    DWORD result_size{sizeof(result)};
    const auto err = RegGetValueW(key,
      subkey,
      name,
      RRF_RT_REG_DWORD,
      NULL, // FIXME: support REG_DWORD_BIG_ENDIAN
      &result,
      &result_size);
    return result_or_throw(std::move(result), err);
  } else if constexpr (is_same_v<Type, std::string> || is_same_v<Type, std::wstring>) {
    constexpr const auto char_size = sizeof(typename Type::value_type);
    static_assert(char_size <= sizeof(wchar_t));
    auto buf_size = required_buffer_size(key, subkey, name);
    if (!buf_size)
      return std::nullopt;
    else if (buf_size % char_size)
      throw std::runtime_error{"cannot get string (REG_SZ) from registry:"
        " incompatible destination string type"};
    Type result(buf_size/char_size - sizeof(wchar_t)/char_size, 0);
    const auto err = RegGetValueW(key,
      subkey,
      name,
      RRF_RT_REG_SZ,
      NULL,
      result.data(),
      &buf_size);
    DMITIGR_ASSERT(buf_size == result.size()*char_size ||
      buf_size == (result.size() + 1)*char_size);
    return result_or_throw(std::move(result), err);
  } else
    static_assert(false_value<T>, "unsupported type specified");
}

/// @overload
template<typename T>
std::optional<T> value(const HKEY key, const std::wstring& subkey,
  const std::wstring& name)
{
  return value<T>(key, subkey.c_str(), name.c_str());
}

} // namespace dmitigr::winbase::registry
