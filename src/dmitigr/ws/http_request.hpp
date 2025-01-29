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

#ifndef DMITIGR_WS_HTTP_REQUEST_HPP
#define DMITIGR_WS_HTTP_REQUEST_HPP

#include "../net/address.hpp"
#include "types_fwd.hpp"

#include <string_view>

namespace dmitigr::ws {

/// A HTTP request.
class Http_request {
public:
  /// The destructor.
  virtual ~Http_request() = default;

  /// @returns The textual representation of the remote IP address.
  virtual const net::Ip_address& remote_ip_address() const noexcept = 0;

  /// @returns The textual representation of the local IP address.
  virtual const net::Ip_address& local_ip_address() const noexcept = 0;

  /// @returns The HTTP request method.
  virtual std::string_view method() const noexcept = 0;

  /// @returns The HTTP request path.
  virtual std::string_view path() const noexcept = 0;

  /// @returns The HTTP request query string.
  virtual std::string_view query_string() const noexcept = 0;

  /// @returns The value of HTTP request header named by `name`.
  virtual std::string_view header(std::string_view name) const noexcept = 0;

private:
  friend detail::iHttp_request;

  Http_request() = default;
};

} // namespace dmitigr::ws

#ifndef DMITIGR_WS_NOT_HEADER_ONLY
#include "http_request.cpp"
#endif

#endif  // DMITIGR_WS_HTTP_REQUEST_HPP
