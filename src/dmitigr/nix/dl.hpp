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

#ifndef DMITIGR_NIX_DL_HPP
#define DMITIGR_NIX_DL_HPP

#include "../base/assert.hpp"
#include "../base/noncopymove.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <dlfcn.h>

namespace dmitigr::nix::dl {

namespace detail {

[[noreturn]] void throw_error(const char* const alt)
{
  DMITIGR_ASSERT(alt);
  const char* const msg = dlerror();
  throw std::runtime_error{msg ? msg : alt};
}

} // namespace detail

template<typename T>
class Symbol final {
  static_assert(std::is_pointer_v<T>);
public:
  using Type = T;

  Symbol() = default;

  explicit Symbol(void* const addr)
  {
    if (!dladdr(addr, &info_))
      detail::throw_error("dladdr() failed");
    DMITIGR_ASSERT(is_valid());
  }

  bool is_valid() const noexcept
  {
    return static_cast<bool>(info_.dli_saddr);
  }

  explicit operator bool() const noexcept
  {
    return is_valid();
  }

  std::filesystem::path object_path() const
  {
    check();
    return info_.dli_fname;
  }

  void* object_address() const
  {
    check();
    return info_.dli_fbase;
  }

  std::string name() const
  {
    check();
    return info_.dli_sname;
  }

  void* address() const
  {
    check();
    return info_.dli_saddr;
  }

  const Dl_info& info() const
  {
    check();
    return info_;
  }

  template<typename ... Types>
  decltype(auto) invoke(Types&& ... args) const
  {
    const auto f = reinterpret_cast<Type>(address());
    return f(std::forward<Types>(args)...);
  }

  template<typename ... Types>
  decltype(auto) operator()(Types&& ... args) const
  {
    return invoke(std::forward<Types>(args)...);
  }

  decltype(auto) value() const
  {
    return *static_cast<Type*>(address());
  }

  decltype(auto) operator*() const
  {
    return *value();
  }

private:
  Dl_info info_{};

  void check() const
  {
    if (!is_valid())
      throw std::invalid_argument{"invalid dmitigr::nix::dl::Symbol"};
  }
};

class Object final : Noncopy {
public:
  ~Object()
  {
    if (handle_)
      dlclose(handle_);
  }

  Object() = default;

  Object(const std::filesystem::path& path, const int flags)
    : handle_{dlopen(path.string().c_str(), flags)}
  {
    if (!handle_)
      detail::throw_error("dlopen() failed");
  }

  Object(Object&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = {};
  }

  Object& operator=(Object&& rhs) noexcept
  {
    Object tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Object& rhs) noexcept
  {
    using std::swap;
    swap(handle_, rhs.handle_);
  }

  void close()
  {
    if (!dlclose(handle_))
      detail::throw_error("dlclose() failed");
  }

  template<typename T>
  Symbol<T> symbol(const char* const name) const
  {
    void* const addr = dlsym(handle_, name);
    return addr ? Symbol<T>{addr} : Symbol<T>{};
  }

  template<typename T>
  Symbol<T> symbol_or_throw(const char* const name) const
  {
    if (void* const addr = dlsym(handle_, name))
      return Symbol<T>{addr};
    else
      detail::throw_error("symbol not found");
  }

private:
  void* handle_{};
};

} // namespace dmitigr::nix::dl

#endif  // DMITIGR_NIX_DL_HPP
