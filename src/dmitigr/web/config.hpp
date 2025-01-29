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

#ifndef DMITIGR_WEB_CONFIG_HPP
#define DMITIGR_WEB_CONFIG_HPP

#include "../rajson/document.hpp"
#include "../ws/server_options.hpp"
#include "basics.hpp"
#include "exceptions.hpp"
#include "rajson.hpp"

#include <algorithm>
#include <filesystem>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <string>

namespace dmitigr::web {

/// Backend configuration.
class Config {
public:
  /// The destructor.
  virtual ~Config() = default;

  /// @returns A value of "server.host" parameter.
  const std::string& server_host() const noexcept
  {
    return server_host_;
  }

  /// @returns A value of "server.port" parameter.
  int server_port() const noexcept
  {
    return server_port_;
  }

  /// @returns A value of "server.ws.idleTimeout" parameter.
  std::chrono::seconds server_ws_idle_timeout() const noexcept
  {
    return server_ws_idle_timeout_;
  }

  /// @returns A value of "server.ws.maxIncomingPayloadSize" parameter.
  std::uint32_t server_ws_max_incoming_payload_size() const noexcept
  {
    return server_ws_max_incoming_payload_size_;
  }

  /// @returns A value of "server.ws.backpressureBufferSize" parameter.
  std::uint32_t server_ws_backpressure_buffer_size() const noexcept
  {
    return server_ws_backpressure_buffer_size_;
  }

  /// @returns A value of "httper.docroot"
  const std::filesystem::path& httper_docroot() const noexcept
  {
    return httper_docroot_;
  }

  /// @returns A value of "threadPoolSize" parameter.
  std::optional<std::uint32_t> thread_pool_size() const noexcept
  {
    return thread_pool_size_;
  }

  /// @returns The default language.
  Language default_language() const noexcept
  {
    return default_language_;
  }

  /// @returns `true` if the instance is behind proxy.
  bool is_behind_proxy() const noexcept
  {
    return is_behind_proxy_;
  }

protected:
  /// Deserializes the config.
  void init(const rajson::Document& cfg)
  {
    using dmitigr::rajson::Path;

    // server.host.
    cfg.get(server_host_, Path{"server/host"});
    if (server_host_.empty() || any_of(cbegin(server_host_),
        cend(server_host_), [](const auto c){return std::isspace(c);}))
      throw Exception{"invalid server.host config parameter"};

    // server.port.
    cfg.get(server_port_, Path{"server/port"});
    if (server_port_ < 0 || server_port_ > 65535)
      throw Exception{"invalid server.port config parameter"};

    // server.ws.idleTimeout.
    cfg.get(server_ws_idle_timeout_,
      Path{"server/ws/idleTimeout"});

    // server.ws.maxIncomingPayloadSize.
    cfg.get(server_ws_max_incoming_payload_size_,
      Path{"server/ws/maxIncomingPayloadSize"});

    // server.ws.backpressureBufferSize.
    cfg.get(server_ws_backpressure_buffer_size_,
      Path{"server/ws/backpressureBufferSize"});

    // httper.docroot
    cfg.get(httper_docroot_, Path{"httper/docroot"});
    if (httper_docroot_.empty())
      throw Exception{"invalid httper.docroot config parameter"};

    // threadPoolSize.
    cfg.get(thread_pool_size_, Path{"threadPoolSize"});

    // defaultLanguage.
    cfg.get(default_language_, Path{"defaultLanguage"});

    // isBehindProxy.
    cfg.get(is_behind_proxy_, Path{"isBehindProxy"});
  }

private:
  std::string server_host_;
  int server_port_{};
  std::chrono::seconds server_ws_idle_timeout_{};
  std::uint32_t server_ws_max_incoming_payload_size_{};
  std::uint32_t server_ws_backpressure_buffer_size_{};
  std::filesystem::path httper_docroot_;
  std::optional<std::uint32_t> thread_pool_size_{};
  Language default_language_;
  bool is_behind_proxy_{};
};

/// @returns A server options from `cfg`.
inline ws::Server_options server_options(const Config& cfg)
{
  return ws::Server_options{}
    .set_http_enabled(true)
    .set_host(cfg.server_host())
    .set_port(cfg.server_port())
    .set_ws_idle_timeout(cfg.server_ws_idle_timeout())
    .set_ws_max_incoming_payload_size(cfg.server_ws_max_incoming_payload_size())
    .set_ws_backpressure_buffer_size(cfg.server_ws_backpressure_buffer_size());
}

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_CONFIG_HPP
