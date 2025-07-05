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

#include "../base/assert.hpp"
#include "http_request.hpp"
#include "uwebsockets.hpp"

namespace dmitigr::ws::detail {

/// The HTTP request implementation.
class iHttp_request final : public Http_request {
public:
  explicit iHttp_request(uWS::HttpRequest* const rep,
    const std::string_view remote_ip_address_binary,
    const std::string_view local_ip_address_binary)
    : rep_{rep}
    , remote_ip_{net::Ip_address::from_binary(remote_ip_address_binary)}
    , local_ip_{net::Ip_address::from_binary(local_ip_address_binary)}
  {
    DMITIGR_ASSERT(rep_);
  }

  const net::Ip_address& remote_ip_address() const noexcept override
  {
    return remote_ip_;
  }

  const net::Ip_address& local_ip_address() const noexcept override
  {
    return local_ip_;
  }

  std::string_view method() const noexcept override
  {
    return rep_->getMethod();
  }

  std::string_view path() const noexcept override
  {
    return rep_->getUrl();
  }

  std::string_view query_string() const noexcept override
  {
    return rep_->getQuery();
  }

  std::string_view header(const std::string_view name) const noexcept override
  {
    return rep_->getHeader(name);
  }

private:
  uWS::HttpRequest* rep_{};
  net::Ip_address remote_ip_;
  net::Ip_address local_ip_;
};

} // namespace dmitigr::ws::detail
