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

#ifndef DMITIGR_WS_UTIL_HPP
#define DMITIGR_WS_UTIL_HPP

#include "../3rdparty/usockets/libusockets_dmitigr.h"
#include "../base/assert.hpp"

#include <string_view>

namespace dmitigr::ws::detail {

/// The data associated with every WebSocket.
struct Ws_data final {
  std::shared_ptr<Connection> conn;
};

/// @returns Local address of the socket.
inline std::string_view local_address(const bool is_ssl, us_socket_t* const s) noexcept
{
  DMITIGR_ASSERT(s);
  static thread_local char buf[16];
  int ip_size = sizeof(buf);
  us_socket_local_address(is_ssl, s, buf, &ip_size);
  return std::string_view{buf, static_cast<std::string_view::size_type>(ip_size)};
}

} // namespace dmitigr::ws::detail

#endif  // DMITIGR_WS_UTIL_HPP
