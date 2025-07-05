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

#ifndef DMITIGR_PGFEX_RAJSON_HPP
#define DMITIGR_PGFEX_RAJSON_HPP

#include "../rajson/conversions_enum.hpp"
#include "../rajson/document.hpp"
#include "../rajson/emplace.hpp"
#include "../pgfe/basics.hpp"

namespace dmitigr::rajson {

/// Full specialization for pgfe::Communication_mode.
template<> struct Enum_traits<pgfe::Communication_mode> final {
  static auto to_type(const std::string_view str) noexcept
  {
    return pgfe::to_communication_mode(str);
  }

  static constexpr const char* singular_name() noexcept
  {
    return "communication mode";
  }
};

/// Full specialization for pgfe::Session_mode.
template<> struct Enum_traits<pgfe::Session_mode> final {
  static auto to_type(const std::string_view str) noexcept
  {
    return pgfe::to_session_mode(str);
  }

  static constexpr const char* singular_name() noexcept
  {
    return "session mode";
  }
};

/// Full specialization for pgfe::Channel_binding.
template<> struct Enum_traits<pgfe::Channel_binding> final {
  static auto to_type(const std::string_view str) noexcept
  {
    return pgfe::to_channel_binding(str);
  }

  static constexpr const char* singular_name() noexcept
  {
    return "channel binding";
  }
};

/// Full specialization for pgfe::Ssl_protocol_version.
template<> struct Enum_traits<pgfe::Ssl_protocol_version> final {
  static auto to_type(const std::string_view str) noexcept
  {
    return pgfe::to_ssl_protocol_version(str);
  }

  static constexpr const char* singular_name() noexcept
  {
    return "SSL protocol version";
  }
};

/// Full specialization for `pgfe::Communication_mode`.
template<>
struct Conversions<pgfe::Communication_mode> final :
    Enum_conversions<pgfe::Communication_mode>{};

/// Full specialization for `pgfe::Session_mode`.
template<>
struct Conversions<pgfe::Session_mode> final :
    Enum_conversions<pgfe::Session_mode>{};

/// Full specialization for `pgfe::Channel_binding`.
template<>
struct Conversions<pgfe::Channel_binding> final :
    Enum_conversions<pgfe::Channel_binding>{};

/// Full specialization for `pgfe::Ssl_protocol_version`.
template<>
struct Conversions<pgfe::Ssl_protocol_version> final :
    Enum_conversions<pgfe::Ssl_protocol_version>{};

} // namespace dmitigr::rajson

#endif  // DMITIGR_PGFEX_RAJSON_HPP
