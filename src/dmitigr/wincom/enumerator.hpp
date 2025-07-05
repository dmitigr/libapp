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

#include "../winbase/combase.hpp"
#include "object.hpp"

#include <optional>

namespace dmitigr::wincom {

template<class EnumInterface, class Item>
class Enumerator final
  : public Unknown_api<Enumerator<EnumInterface, Item>, EnumInterface> {
  using Ua = Unknown_api<Enumerator<EnumInterface, Item>, EnumInterface>;
public:
  using Ua::Ua;

  explicit Enumerator(IUnknown* const api)
  {
    auto instance = Ua::query(api);
    Ua::swap(instance);
  }

  Enumerator clone() const
  {
    EnumInterface* instance{};
    const auto err = detail::api(*this).Clone(&instance);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot clone enumerator");
    return Enumerator{instance};
  }

  std::optional<Item> next()
  {
    Item result{};
    ULONG fetched{};
    Ua::api().Next(1, &result, &fetched);
    return fetched ? std::make_optional(result) : std::nullopt;
  }

  template<class T>
  std::optional<T> query_next()
  {
    static_assert(std::is_same_v<Item, IUnknown*>);
    auto i = next();
    return i ? std::make_optional(T::query(*i)) : std::nullopt;
  }

  void reset()
  {
    const auto err = Ua::api().Reset();
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot reset enumerator");
  }

  void skip(const ULONG count)
  {
    const auto err = Ua::api().Skip(count);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot skip enumerator");
  }
};

} // namespace dmitigr::wincom
