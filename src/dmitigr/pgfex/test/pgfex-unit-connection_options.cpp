// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin

#include "../../base/assert.hpp"
#include "../../pgfex/connection_options.hpp"
#include "../../rajson/document.hpp"

#include <chrono>
#include <iostream>
#include <string>

#define ASSERT DMITIGR_ASSERT

int main()
{
  namespace pgfe = dmitigr::pgfe;
  namespace pgfex = dmitigr::pgfex;
  namespace rajson = dmitigr::rajson;
  using std::chrono::seconds;
  using std::chrono::milliseconds;
  try {
    const rajson::Document doc{std::string_view{R"(
{
  "communicationMode":"net",
  "sessionMode":"primary",
  "connectTimeout":1983,
  "waitResponseTimeout":1988,
  "port":9876,
  "uds": {
    "directory":"/path/to/uds/directory",
    "requireServerProcessUsername":"dmitigr"
  },
  "tcp": {
    "keepalivesIdle": 10,
    "keepalivesInterval": 20,
    "keepalivesCount": 30,
    "userTimeout": 40
  },
  "address":"127.0.0.1",
  "hostname":"localhost",
  "username":"dmitigr",
  "database":"dmitigr",
  "password":"dmitigr",
  "passwordFile":"/path/to/password/file",
  "channelBinding":"required",
  "kerberosServiceName":"kerberos",
  "ssl": {
    "isEnabled":true,
    "minProtocolVersion":"tls1.1",
    "maxProtocolVersion":"tls1.3",
    "isCompressionEnabled": false,
    "certificateFile":"/path/to/certificate/file",
    "privateKeyFile":"/path/to/private/key/file",
    "privateKeyFilePassword":"secret",
    "certificateAuthorityFile":"/path/to/certificate/authority/file",
    "certificateRevocationListFile":"/path/to/certificate/revocation/list/file",
    "isServerHostnameVerificationEnabled": true,
    "isServerNameIndicationEnabled": false
  }
}
)"}};
    const auto opts = pgfex::to_connection_options(doc.json());
    ASSERT(opts.communication_mode() == pgfe::Communication_mode::net);
    ASSERT(opts.session_mode() == pgfe::Session_mode::primary);
    ASSERT(opts.connect_timeout() == milliseconds{1983});
    ASSERT(opts.wait_response_timeout() == milliseconds{1988});
    ASSERT(opts.port() == 9876);
    // uds
    ASSERT(opts.uds_directory() == "/path/to/uds/directory");
    ASSERT(opts.uds_require_server_process_username() == "dmitigr");
    // tcp
    ASSERT(opts.tcp_keepalives_idle() == seconds{10});
    ASSERT(opts.tcp_keepalives_interval() == seconds{20});
    ASSERT(opts.tcp_keepalives_count() == 30);
    ASSERT(opts.tcp_user_timeout() == milliseconds{40});
    //
    ASSERT(opts.address() == "127.0.0.1");
    ASSERT(opts.hostname() == "localhost");
    ASSERT(opts.username() == "dmitigr");
    ASSERT(opts.database() == "dmitigr");
    ASSERT(opts.password() == "dmitigr");
    ASSERT(opts.password_file() == "/path/to/password/file");
    ASSERT(opts.channel_binding() == pgfe::Channel_binding::required);
    ASSERT(opts.kerberos_service_name() == "kerberos");
    // ssl
    ASSERT(opts.is_ssl_enabled() == true);
    ASSERT(opts.ssl_min_protocol_version() == pgfe::Ssl_protocol_version::tls1_1);
    ASSERT(opts.ssl_max_protocol_version() == pgfe::Ssl_protocol_version::tls1_3);
    ASSERT(opts.is_ssl_compression_enabled() == false);
    ASSERT(opts.ssl_certificate_file() == "/path/to/certificate/file");
    ASSERT(opts.ssl_private_key_file() == "/path/to/private/key/file");
    ASSERT(opts.ssl_private_key_file_password() == "secret");
    ASSERT(opts.ssl_certificate_authority_file() ==
      "/path/to/certificate/authority/file");
    ASSERT(opts.ssl_certificate_revocation_list_file() ==
      "/path/to/certificate/revocation/list/file");
    ASSERT(opts.is_ssl_server_hostname_verification_enabled() == true);
    ASSERT(opts.is_ssl_server_name_indication_enabled() == false);

    {
      const auto d = rajson::to_document(opts);
      std::cout << rajson::to_text(d) << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
