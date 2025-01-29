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

#ifndef DMITIGR_WEB_RAJSON_HPP
#define DMITIGR_WEB_RAJSON_HPP

#include "../rajson/conversions_enum.hpp"
#include "basics.hpp"

namespace dmitigr::rajson {

/// Full specialization for Language.
template<> struct Enum_traits<web::Language> final {
  static auto to_type(const std::string_view str) noexcept
  {
    return web::to_language(str);
  }

  static constexpr const char* singular_name() noexcept
  {
    return "language";
  }
};

/// Full specialization for `web::Language`.
template<>
struct Conversions<web::Language> final : Enum_conversions<web::Language>{};

} // namespace dmitigr::rajson

#endif  // DMITIGR_WEB_RAJSON_HPP
