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

#ifndef DMITIGR_WEB_BASICS_HPP
#define DMITIGR_WEB_BASICS_HPP

#include "exceptions.hpp"

#include <optional>
#include <string_view>

namespace dmitigr::web {

/// A language.
enum class Language {
  en = 0,
  ru = 100
};

/// @returns The language by `str`.
inline std::optional<Language> to_language(const std::string_view str) noexcept
{
  using L = Language;
  if (str == "en")
    return L::en;
  else if (str == "ru")
    return L::ru;
  else
    return std::nullopt;
}

/// @returns The string representation of `lang`, or `nullptr`.
inline const char* to_literal(const Language lang) noexcept
{
  switch (lang) {
  case Language::en: return "en";
  case Language::ru: return "ru";
  }
  return nullptr;
}

/// @returns The string representation of `lang`, or an empty view.
inline std::string_view to_string_view(const Language lang)
{
  if (const char* const l = to_literal(lang))
    return std::string_view{l};
  else
    throw Exception{"invalid native representation of Language"};
}

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_BASICS_HPP
