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

#ifndef DMITIGR_PGFEX_CONNECTION_OPTIONS_HPP
#define DMITIGR_PGFEX_CONNECTION_OPTIONS_HPP

#include "../pgfe/connection_options.hpp"
#include "rajson.hpp"

namespace dmitigr::pgfex {

/// @returns An instance of type pgfe::Connection_options from `json`.
template<class V>
inline pgfe::Connection_options to_connection_options(const rajson::Value_view<V>& json)
{
  using P = rajson::Path;
  namespace chrono = std::chrono;
  namespace fs = std::filesystem;
  namespace pgfe = dmitigr::pgfe;
  return pgfe::Connection_options{}
    .set_communication_mode(
      json.template optional<pgfe::Communication_mode>(P{"communicationMode"}))
    .set_session_mode(
      json.template optional<pgfe::Session_mode>(P{"sessionMode"}))
    .set_connect_timeout(
      json.template optional<chrono::milliseconds>(P{"connectTimeout"}))
    .set_wait_response_timeout(
      json.template optional<chrono::milliseconds>(P{"waitResponseTimeout"}))
    .set_port(
      json.template optional<std::int_fast32_t>(P{"port"}))
    .set_uds_directory(
      json.template optional<fs::path>(P{"uds/directory"}))
    .set_uds_require_server_process_username(
      json.template optional<std::string>(P{"uds/requireServerProcessUsername"}))
    .set_tcp_keepalives_idle(
      json.template optional<chrono::seconds>(P{"tcp/keepalivesIdle"}))
    .set_tcp_keepalives_interval(
      json.template optional<chrono::seconds>(P{"tcp/keepalivesInterval"}))
    .set_tcp_keepalives_count(
      json.template optional<int>(P{"tcp/keepalivesCount"}))
    .set_tcp_user_timeout(
      json.template optional<chrono::milliseconds>(P{"tcp/userTimeout"}))
    .set_address(
      json.template optional<std::string>(P{"address"}))
    .set_hostname(
      json.template optional<std::string>(P{"hostname"}))
    .set_username(
      json.template optional<std::string>(P{"username"}))
    .set_database(
      json.template optional<std::string>(P{"database"}))
    .set_password(
      json.template optional<std::string>(P{"password"}))
    .set_password_file(
      json.template optional<fs::path>(P{"passwordFile"}))
    .set_channel_binding(
      json.template optional<pgfe::Channel_binding>(P{"channelBinding"}))
    .set_kerberos_service_name(
      json.template optional<std::string>(P{"kerberosServiceName"}))
    .set_ssl_enabled(
      json.template optional<bool>(P{"ssl/isEnabled"}))
    .set_ssl_min_protocol_version(
      json.template optional<pgfe::Ssl_protocol_version>(P{"ssl/minProtocolVersion"}))
    .set_ssl_max_protocol_version(
      json.template optional<pgfe::Ssl_protocol_version>(P{"ssl/maxProtocolVersion"}))
    .set_ssl_compression_enabled(
      json.template optional<bool>(P{"ssl/isCompressionEnabled"}))
    .set_ssl_certificate_file(
      json.template optional<fs::path>(P{"ssl/certificateFile"}))
    .set_ssl_private_key_file(
      json.template optional<fs::path>(P{"ssl/privateKeyFile"}))
    .set_ssl_private_key_file_password(
      json.template optional<std::string>(P{"ssl/privateKeyFilePassword"}))
    .set_ssl_certificate_authority_file(
      json.template optional<fs::path>(P{"ssl/certificateAuthorityFile"}))
    .set_ssl_certificate_revocation_list_file(
      json.template optional<fs::path>(P{"ssl/certificateRevocationListFile"}))
    .set_ssl_server_hostname_verification_enabled(
      json.template optional<bool>(P{"ssl/isServerHostnameVerificationEnabled"}))
    .set_ssl_server_name_indication_enabled(
      json.template optional<bool>(P{"ssl/isServerNameIndicationEnabled"}));
}

template<class Encoding, class Allocator>
rapidjson::GenericValue<Encoding, Allocator>
to_json(const pgfe::Connection_options& opts, Allocator& alloc)
{
  rapidjson::GenericValue<Encoding, Allocator> result{rapidjson::kObjectType};
  using rajson::emplace;
  using P = rajson::Path;
  emplace(result, alloc, P{"communicationMode"},
    opts.communication_mode());
  emplace(result, alloc, P{"sessionMode"},
    opts.session_mode());
  emplace(result, alloc, P{"connectTimeout"},
    opts.connect_timeout());
  emplace(result, alloc, P{"waitResponseTimeout"},
    opts.wait_response_timeout());
  emplace(result, alloc, P{"port"},
    opts.port());
  emplace(result, alloc, P{"uds/directory"},
    opts.uds_directory());
  emplace(result, alloc, P{"uds/requireServerProcessUsername"},
    opts.uds_require_server_process_username());
  emplace(result, alloc, P{"tcp/keepalivesIdle"},
    opts.tcp_keepalives_idle());
  emplace(result, alloc, P{"tcp/keepalivesInterval"},
    opts.tcp_keepalives_interval());
  emplace(result, alloc, P{"tcp/keepalivesCount"},
    opts.tcp_keepalives_count());
  emplace(result, alloc, P{"tcp/userTimeout"},
    opts.tcp_user_timeout());
  emplace(result, alloc, P{"address"},
    opts.address());
  emplace(result, alloc, P{"hostname"},
    opts.hostname());
  emplace(result, alloc, P{"username"},
    opts.username());
  emplace(result, alloc, P{"database"},
    opts.database());
  emplace(result, alloc, P{"password"},
    opts.password());
  emplace(result, alloc, P{"passwordFile"},
    opts.password_file());
  emplace(result, alloc, P{"channelBinding"},
    opts.channel_binding());
  emplace(result, alloc, P{"kerberosServiceName"},
    opts.kerberos_service_name());
  emplace(result, alloc, P{"ssl/isEnabled"},
    opts.is_ssl_enabled());
  emplace(result, alloc, P{"ssl/minProtocolVersion"},
    opts.ssl_min_protocol_version());
  emplace(result, alloc, P{"ssl/maxProtocolVersion"},
    opts.ssl_max_protocol_version());
  emplace(result, alloc, P{"ssl/isCompressionEnabled"},
    opts.is_ssl_compression_enabled());
  emplace(result, alloc, P{"ssl/certificateFile"},
    opts.ssl_certificate_file());
  emplace(result, alloc, P{"ssl/privateKeyFile"},
    opts.ssl_private_key_file());
  emplace(result, alloc, P{"ssl/privateKeyFilePassword"},
    opts.ssl_private_key_file_password());
  emplace(result, alloc, P{"ssl/certificateAuthorityFile"},
    opts.ssl_certificate_authority_file());
  emplace(result, alloc, P{"ssl/certificateRevocationListFile"},
    opts.ssl_certificate_revocation_list_file());
  emplace(result, alloc, P{"ssl/isServerHostnameVerificationEnabled"},
    opts.is_ssl_server_hostname_verification_enabled());
  emplace(result, alloc, P{"ssl/isServerNameIndicationEnabled"},
    opts.is_ssl_server_name_indication_enabled());
  return result;
}

} // namespace dmitigr::pgfex

namespace dmitigr::rajson {

/// Full specialization for `pgfe::Connection_options`.
template<>
struct Conversions<pgfe::Connection_options> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return pgfex::to_connection_options(Value_view{value});
  }

  template<class Encoding, class Allocator>
  static auto to_value(const pgfe::Connection_options& obj, Allocator& alloc)
  {
    return pgfex::to_json<Encoding, Allocator>(obj, alloc);
  }
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_PGFEX_CONNECTION_OPTIONS_HPP
