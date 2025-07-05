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

#ifndef DMITIGR_MAC_CF_HPP
#define DMITIGR_MAC_CF_HPP

#ifndef __APPLE__
#error dmitigr/mac/cf.hpp is usable only on macOS!
#endif

#include "../base/traits.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <CoreFoundation/CoreFoundation.h>

namespace dmitigr::mac::cf {

// -----------------------------------------------------------------------------
// Traits
// -----------------------------------------------------------------------------

template<typename> struct Traits;

template<> struct Traits<CFNumberRef> final {
  static CFTypeID cf_type_id() noexcept
  {
    return CFNumberGetTypeID();
  }
};

template<> struct Traits<CFStringRef> final {
  static CFTypeID cf_type_id() noexcept
  {
    return CFStringGetTypeID();
  }
};

template<> struct Traits<char> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberCharType};
};

template<> struct Traits<short> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberShortType};
};

template<> struct Traits<int> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberIntType};
};

template<> struct Traits<long> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberLongType};
};

template<> struct Traits<long long> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberLongLongType};
};

template<> struct Traits<float> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberFloatType};
};

template<> struct Traits<double> final {
  using Ref = CFNumberRef;

  static constexpr CFNumberType number_type{kCFNumberDoubleType};
};

template<> struct Traits<std::string> final {
  using Ref = CFStringRef;
};

// -----------------------------------------------------------------------------
// Handle
// -----------------------------------------------------------------------------

template<class RefType>
class [[nodiscard]] Handle final {
public:
  using Ref = RefType;

  ~Handle()
  {
    if (native_)
      CFRelease(native_);
  }

  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;

  Handle() = default;

  static Handle created(Ref native)
  {
    return Handle{native};
  }

  static Handle retained(Ref native)
  {
    if (native)
      CFRetain(native);
    return Handle{native};
  }

  Handle(Handle&& rhs) noexcept
    : native_{rhs.native_}
  {
    rhs.native_ = {};
  }

  Handle& operator=(Handle&& rhs) noexcept
  {
    Handle tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Handle& rhs) noexcept
  {
    using std::swap;
    swap(native_, rhs.native_);
  }

  Ref native() const noexcept
  {
    return native_;
  }

  explicit operator bool() const noexcept
  {
    return static_cast<bool>(native());
  }

private:
  Ref native_{};

  explicit Handle(Ref native) noexcept
    : native_{native}
  {}
};

using Bundle = Handle<CFBundleRef>;
using Dictionary = Handle<CFDictionaryRef>;
using Number = Handle<CFNumberRef>;
using String = Handle<CFStringRef>;
using Url = Handle<CFURLRef>;

template<typename>
struct Is_handle : std::false_type {};

template<typename T>
struct Is_handle<Handle<T>> : std::true_type {};

template<typename T>
constexpr bool Is_handle_v = Is_handle<T>::value;

// -----------------------------------------------------------------------------
// Number
// -----------------------------------------------------------------------------

inline namespace number {

template<typename T>
[[nodiscard]]
Number create(const T value)
{
  using D = std::decay_t<T>;
  return Number::created(CFNumberCreate(kCFAllocatorDefault,
    Traits<D>::number_type, &value));
}

template<typename T>
[[nodiscard]]
std::pair<T, bool> to_approximated(const Number& number)
{
  if (!number)
    throw std::invalid_argument{"cannot access number: invalid handle"};

  using D = std::decay_t<T>;
  D result{};
  const auto ok = CFNumberGetValue(number.native(), Traits<D>::number_type,
    &result);
  return {result, ok};
}

template<typename T>
[[nodiscard]]
T to(const Number& number)
{
  using D = std::decay_t<T>;
  const auto [result, ok] = to_approximated<T>(number);
  if (!ok)
    throw std::runtime_error{"cannot convert CFNumber to value of CFNumberType "
      +std::to_string(Traits<D>::number_type)};
  return result;
}

} // inline namespace number

// -----------------------------------------------------------------------------
// String
// -----------------------------------------------------------------------------

inline namespace string {

[[nodiscard]]
inline String create_no_copy(const char* const str,
  const CFStringEncoding encoding = kCFStringEncodingUTF8)
{
  return String::created(CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, str,
    encoding, kCFAllocatorNull));
}

[[nodiscard]]
inline std::string to_string(const String& str,
  const CFStringEncoding result_encoding = kCFStringEncodingUTF8)
{
  if (!str)
    throw std::invalid_argument{"cannot access string: invalid handle"};

  {
    const char* const c_str = CFStringGetCStringPtr(str.native(),
      result_encoding);
    if (c_str)
      return c_str;
  }

  const auto utf16_chars_count = CFStringGetLength(str.native());
  const auto bytes_count = CFStringGetMaximumSizeForEncoding(utf16_chars_count,
    result_encoding);
  if (bytes_count == kCFNotFound)
    throw std::overflow_error{"cannot convert CFString to std::string"};

  std::string result(bytes_count, 0);
  if (!CFStringGetCString(str.native(), result.data(), result.size() + 1,
      result_encoding))
    throw std::runtime_error{"cannot convert CFString to std::string"};

  const auto e = find_if_not(result.crbegin(), result.crend(),
    [](const auto ch){return !ch;}).base();
  result.resize(e - result.cbegin());
  return result;
}

} // inline namespace string

// -----------------------------------------------------------------------------
// Bundle
// -----------------------------------------------------------------------------

inline namespace bundle {

[[nodiscard]]
inline Bundle create(const Url& url)
{
  return Bundle::created(CFBundleCreate(kCFAllocatorDefault, url.native()));
}

[[nodiscard]]
inline Bundle create(const std::filesystem::path& path)
{
  const auto path_hdl = string::create_no_copy(path.c_str());
  const auto url = Url::created(CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
    path_hdl.native(), kCFURLPOSIXPathStyle, is_directory(path)));
  return bundle::create(url);
}

[[nodiscard]]
inline void* function_pointer_for_name(const Bundle& bundle,
  const char* const name)
{
  if (!bundle)
    throw std::invalid_argument{"cannot access bundle: invalid handle"};

  return CFBundleGetFunctionPointerForName(bundle.native(),
    string::create_no_copy(name).native());
}

} // inline namespace bundle

// -----------------------------------------------------------------------------
// Dictionary
// -----------------------------------------------------------------------------

inline namespace dictionary {

[[nodiscard]]
inline Dictionary create(const void** keys, const void** values,
  const CFIndex size,
  const CFDictionaryKeyCallBacks* const key_callbacks,
  const CFDictionaryValueCallBacks* const value_callbacks)
{
  return Dictionary::created(CFDictionaryCreate(kCFAllocatorDefault, keys,
    values, size, key_callbacks, value_callbacks));
}

[[nodiscard]]
inline std::optional<const void*> value(const Dictionary& dictionary,
  const void* const key)
{
  if (!dictionary)
    throw std::invalid_argument{"cannot access dictionary: invalid handle"};

  const void* result{};
  if (CFDictionaryGetValueIfPresent(dictionary.native(), key, &result))
    return result;
  else
    return std::nullopt;
}

template<typename ValueHdl, typename KeyRef>
[[nodiscard]]
std::enable_if_t<Is_handle_v<std::decay_t<ValueHdl>>, std::optional<ValueHdl>>
value(const Dictionary& dictionary, const Handle<KeyRef>& key)
{
  if (const auto val = value(dictionary, key.native())) {
    using Value_ref = typename ValueHdl::Ref;
    if (CFGetTypeID(*val) != Traits<Value_ref>::cf_type_id())
      throw std::runtime_error{"cannot get value of dictionary: "
        "incompatible value type"};
    return ValueHdl::retained(static_cast<Value_ref>(*val));
  }
  return std::nullopt;
}

template<typename Value, typename Key>
[[nodiscard]]
std::optional<Value> value(const Dictionary& dictionary, const Key& key)
{
  using Dkey = std::decay_t<Key>;
  using Dvalue = std::decay_t<Value>;

  using Value_hdl = std::conditional_t<Is_handle_v<Dvalue>, Dvalue,
    Handle<typename Traits<Dvalue>::Ref>>;

  const auto& key_hdl = [&key]() -> decltype(auto)
  {
    if constexpr (Is_handle_v<Dkey>)
      return key;
    else if constexpr (std::is_arithmetic_v<Dkey>)
      return number::create(key);
    else if constexpr (std::is_same_v<Dkey, std::string>)
      return string::create_no_copy(key.c_str());
    else
      static_assert(false_value<Dkey>, "unsupported key type");
  }();

  if (const auto val = value<Value_hdl>(dictionary, key_hdl)) {
    if constexpr (Is_handle_v<Dvalue>)
      return *val;
    else if constexpr (std::is_arithmetic_v<Dvalue>)
      return number::to<Dvalue>(*val);
    else if constexpr (std::is_same_v<Dvalue, std::string>)
      return string::to_string(*val);
    else
      static_assert(false_value<Dvalue>, "unsupported value type");
  }

  return std::nullopt;
}

template<typename Value>
[[nodiscard]]
std::optional<Value> value(const Dictionary& dictionary, const char* const key)
{
  return value<Value>(dictionary, string::create_no_copy(key));
}

} // inline namespace dictionary

} // namespace dmitigr::mac::cf

#endif  // DMITIGR_MAC_CF_HPP
