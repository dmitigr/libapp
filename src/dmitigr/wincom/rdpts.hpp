// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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

#include "../winbase/windows.hpp"
#include "exceptions.hpp"
#include "object.hpp"

#include <chrono>
#include <type_traits>

#import "libid:8C11EFA1-92C3-11D1-BC1E-00C04FA31489"
#include <mstscax.tlh>

namespace dmitigr::wincom::rdpts {

// -----------------------------------------------------------------------------
// Advanced_settings
// -----------------------------------------------------------------------------

class Advanced_settings final
  : public Unknown_api<Advanced_settings, MSTSCLib::IMsRdpClientAdvancedSettings8> {
  using Ua = Unknown_api<Advanced_settings, MSTSCLib::IMsRdpClientAdvancedSettings8>;
public:
  using Ua::Ua;

  enum class Server_authentication : UINT {
    /// No authentication of the server.
    disabled = 0,
    /// Server authentication is required.
    required = 1,
    /// Attempt authentication of the server.
    prompted = 2
  };

  enum class Network_connection_type : UINT {
    /// 56 Kbps.
    modem = 1,
    /// 256 Kbps to 2 Mbps.
    low = 2,
    /// 2 Mbps to 16 Mbps, with high latency.
    satellite = 3,
    /// 2 Mbps to 10 Mbps.
    broadband_high = 4,
    /// 10 Mbps or higher, with high latency.
    wan = 5,
    /// 10 Mbps or higher.
    lan = 6
  };

  // IMsRdpClientAdvancedSettings

  void set_rdp_port(const LONG value)
  {
    detail::set("RDP port", *this, &Api::put_RDPPort, value);
  }

  LONG rdp_port() const
  {
    return detail::get("RDP port", *this, &Api::get_RDPPort);
  }

  void set_smart_sizing_enabled(const bool value)
  {
    detail::set("smart sizing enabled",
      *this, &Api::put_SmartSizing, value);
  }

  bool is_smart_sizing_enabled() const noexcept
  {
    return detail::get_bool("smart sizing enabled", *this, &Api::get_SmartSizing);
  }

  void set_overall_connection_timeout(const std::chrono::seconds value)
  {
    detail::set("overall connection timeout",
      *this, &Api::put_overallConnectionTimeout, value.count());
  }

  std::chrono::seconds overall_connection_timeout() const
  {
    return std::chrono::seconds{detail::get("overall connection timeout",
      *this, &Api::get_overallConnectionTimeout)};
  }

  void set_single_connection_timeout(const std::chrono::seconds value)
  {
    const auto err = api().put_singleConnectionTimeout(value.count());
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set single connection timeout");
  }

  std::chrono::seconds single_connection_timeout() const
  {
    LONG result{};
    const auto err = detail::api(*this).get_singleConnectionTimeout(&result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get single connection timeout");
    return std::chrono::seconds{result};
  }

  void set_shutdown_timeout(const std::chrono::seconds value)
  {
    const auto err = api().put_shutdownTimeout(value.count());
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set shutdown timeout");
  }

  std::chrono::seconds shutdown_timeout() const
  {
    LONG result{};
    const auto err = detail::api(*this).get_shutdownTimeout(&result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get shutdown timeout");
    return std::chrono::seconds{result};
  }

  void set_idle_timeout(const std::chrono::minutes value)
  {
    const auto err = api().put_MinutesToIdleTimeout(value.count());
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set idle timeout");
  }

  std::chrono::minutes idle_timeout() const
  {
    LONG result{};
    const auto err = detail::api(*this).get_MinutesToIdleTimeout(&result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get idle timeout");
    return std::chrono::minutes{result};
  }

  void set_redirect_drives_enabled(const bool value)
  {
    detail::set("redirect drives enabled",
      *this, &Api::put_RedirectDrives, value);
  }

  bool is_redirect_drives_enabled() const
  {
    return detail::get_bool("redirect drives enabled",
      *this, &Api::get_RedirectDrives);
  }

  void set_redirect_ports_enabled(const bool value)
  {
    detail::set("redirect ports enabled",
      *this, &Api::put_RedirectPorts, value);
  }

  bool is_redirect_port_enabled() const
  {
    return detail::get_bool("redirect ports enabled",
      *this, &Api::get_RedirectPorts);
  }

  void set_redirect_printers_enabled(const bool value)
  {
    detail::set("redirect printers enabled",
      *this, &Api::put_RedirectPrinters, value);
  }

  bool is_redirect_printers_enabled() const
  {
    return detail::get_bool("redirect printers enabled",
      *this, &Api::get_RedirectPrinters);
  }

  void set_redirect_smart_cards_enabled(const bool value)
  {
    detail::set("redirect smart cards enabled",
      *this, &Api::put_RedirectSmartCards, value);
  }

  bool is_redirect_smart_cards_enabled() const
  {
    return detail::get_bool("redirect smart cards enabled",
      *this, &Api::get_RedirectSmartCards);
  }

  /// @param value The minimum valid value is `10000`.
  void set_keep_alive_interval(const std::chrono::milliseconds value)
  {
    const auto err = api().put_keepAliveInterval(value.count());
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set keep-alive interval");
  }

  std::chrono::milliseconds keep_alive_interval() const
  {
    LONG result{};
    const auto err = detail::api(*this).get_keepAliveInterval(&result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get keep-alive interval");
    return std::chrono::milliseconds{result};
  }

  // IMsRdpClientAdvancedSettings2

  void set_auto_reconnect_enabled(const bool value)
  {
    detail::set("auto reconnect enabled",
      *this, &Api::put_EnableAutoReconnect, value);
  }

  bool is_auto_reconnect_enabled() const
  {
    return detail::get_bool("auto reconnect enabled",
      *this, &Api::get_EnableAutoReconnect);
  }

  void set_max_reconnect_attempts(const LONG value)
  {
    const auto err = detail::api(*this).put_MaxReconnectAttempts(value);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set max reconnect attempts");
  }

  LONG max_reconnect_attempts() const
  {
    LONG result{};
    const auto err = detail::api(*this).get_MaxReconnectAttempts(&result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get max reconnect attempts");
    return result;
  }

  // IMsRdpClientAdvancedSettings4

  void set_authentication_level(const Server_authentication value)
  {
    const auto err = api().put_AuthenticationLevel(
      static_cast<std::underlying_type_t<Server_authentication>>(value));
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set authentication level");
  }

  void set(const Server_authentication value)
  {
    set_authentication_level(value);
  }

  Server_authentication authentication_level() const
  {
    UINT result{};
    detail::api(*this).get_AuthenticationLevel(&result);
    return Server_authentication{result};
  }

  // IMsRdpClientAdvancedSettings5

  void set_redirect_clipboard_enabled(const bool value)
  {
    detail::set("redirect clipboard enabled",
      *this, &Api::put_RedirectClipboard, value);
  }

  bool is_redirect_clipboard_enabled() const
  {
    return detail::get_bool("redirect clipboard enabled",
      *this, &Api::get_RedirectClipboard);
  }

  void set_redirect_devices_enabled(const bool value)
  {
    detail::set("redirect devices enabled",
      *this, &Api::put_RedirectDevices, value);
  }

  bool is_redirect_devices_enabled() const
  {
    return detail::get_bool("redirect devices enabled",
      *this, &Api::get_RedirectDevices);
  }

  void set_redirect_pos_devices_enabled(const bool value)
  {
    detail::set("redirect POS devices enabled",
      *this, &Api::put_RedirectPOSDevices, value);
  }

  bool is_redirect_pos_devices_enabled() const
  {
    return detail::get_bool("redirect POS devices enabled",
      *this, &Api::get_RedirectPOSDevices);
  }

  // IMsRdpClientAdvancedSettings7

  void set_network_connection_type(const Network_connection_type value)
  {
    const auto err = api().put_NetworkConnectionType(
      static_cast<std::underlying_type_t<Network_connection_type>>(value));
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot set network connection type");
  }

  void set(const Network_connection_type value)
  {
    set_network_connection_type(value);
  }

  Network_connection_type network_connection_type() const
  {
    UINT result{};
    detail::api(*this).get_NetworkConnectionType(&result);
    return Network_connection_type{result};
  }
};

// -----------------------------------------------------------------------------
// Client
// -----------------------------------------------------------------------------

class Client_event_dispatcher : public Advise_sink<MSTSCLib::IMsTscAxEvents> {};

class Client final : public Basic_com_object<
  MSTSCLib::MsRdpClient11NotSafeForScripting, MSTSCLib::IMsRdpClient10> {
  using Bco = Basic_com_object<MSTSCLib::MsRdpClient11NotSafeForScripting,
    MSTSCLib::IMsRdpClient10>;
public:
  using Bco::Bco;

  using Event_dispatcher = Client_event_dispatcher;

  ~Client()
  {
    try {
      disconnect();
    } catch (...) {}
  }

  explicit Client(std::unique_ptr<Event_dispatcher> sink)
    : sink_{api(), std::move(sink), this}
  {
    /*
     * Closing a window in which this ActiveX object is hosted leads to
     * releasing it, which causes failure upon of calling api().Release()
     * from ~Bco(). Thus, api.AddRef() call is required here.
     */
    api().AddRef();
  }

  Advanced_settings advanced_settings() const
  {
    MSTSCLib::IMsRdpClientAdvancedSettings8* result{};
    detail::api(*this).get_AdvancedSettings9(&result);
    return Advanced_settings{result};
  }

  // MsTscAxNotSafeForScripting

  template<class String>
  String version() const
  {
    return detail::get<String>(*this, &Api::get_Version);
  }

  /**
   * @param value DNS name or IP address.
   *
   * @remarks Must be set before calling the connect().
   */
  template<class String>
  void set_server(const String& value)
  {
    const auto err = api().put_Server(detail::bstr(value));
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot set Server property of RDP client");
  }

  template<class String>
  String server() const
  {
    return detail::get<String>(*this, &Api::get_Server);
  }

  template<class String>
  void set_user_name(const String& value)
  {
    const auto err = api().put_UserName(detail::bstr(value));
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot set UserName property of RDP client");
  }

  template<class String>
  String user_name() const
  {
    return detail::get<String>(*this, &Api::get_UserName);
  }

  template<class String>
  void set_domain(const std::string& value)
  {
    detail::set("Domain property of RDP client",
      *this, &Api::put_Domain, value);
  }

  template<class String>
  String domain() const
  {
    return detail::get<String>(*this, &Api::get_Domain);
  }

  void set_prompt_for_credentials_enabled(const bool value)
  {
    detail::set("PromptForCredentials property of RDP client", *this,
      &MSTSCLib::IMsRdpClientNonScriptable3::put_PromptForCredentials, value);
  }

  bool is_prompt_for_credentials_enabled() const noexcept
  {
    return detail::get_bool("PromptForCredentials property of RDP client",
      *this, &MSTSCLib::IMsRdpClientNonScriptable3::get_PromptForCredentials);
  }

  void set_prompt_for_credentials_on_client_enabled(const bool value)
  {
    detail::set("PromptForCredsOnClient property of RDP client", *this,
      &MSTSCLib::IMsRdpClientNonScriptable4::put_PromptForCredsOnClient, value);
  }

  bool is_prompt_for_credentials_on_client_enabled() const noexcept
  {
    return detail::get_bool("PromptForCredsOnClient property of RDP client",
      *this, &MSTSCLib::IMsRdpClientNonScriptable4::get_PromptForCredsOnClient);
  }

  void set_allow_prompting_for_credentials_enabled(const bool value)
  {
    detail::set("AllowPromptingForCredentials property of RDP client", *this,
      &MSTSCLib::IMsRdpClientNonScriptable5::put_AllowPromptingForCredentials,
      value);
  }

  bool is_allow_prompting_for_credentials_enabled() const noexcept
  {
    return detail::get_bool("AllowPromptingForCredentials property of RDP client",
      *this,
      &MSTSCLib::IMsRdpClientNonScriptable5::get_AllowPromptingForCredentials);
  }

  void set_desktop_height(const LONG value)
  {
    const auto err = api().put_DesktopHeight(value);
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot set DesktopHeight property of RDP client");
  }

  LONG desktop_height() const
  {
    LONG result{};
    detail::api(*this).get_DesktopHeight(&result);
    return result;
  }

  void set_desktop_width(const LONG value)
  {
    const auto err = api().put_DesktopWidth(value);
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot set DesktopWidth property of RDP client");
  }

  LONG desktop_width() const
  {
    LONG result{};
    detail::api(*this).get_DesktopWidth(&result);
    return result;
  }

  short connection_state() const
  {
    short result{};
    detail::api(*this).get_Connected(&result);
    return result;
  }

  void connect()
  {
    const auto err = api().Connect();
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot initiate connection to remote RDP server");
  }

  void disconnect()
  {
    const auto err = api().Disconnect();
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot disconnect from remote RDP server");
  }

  MSTSCLib::ControlReconnectStatus reconnect(const ULONG width, const ULONG height)
  {
    return api().Reconnect(width, height);
  }

  // MsRdpClient9NotSafeForScripting

  /**
   * @remarks This method:
   *   - will throw if not logged into the user session;
   *   - may throw until some amount of time have elapsed after logging into
   *   the user session.
   */
  void update_session_display_settings(const ULONG desktop_width,
    const ULONG desktop_height, const ULONG physical_width,
    const ULONG physical_height, const ULONG orientation,
    const ULONG desktop_scale_factor, const ULONG device_scale_factor)
  {
    const auto err = [&]
    {
      try {
        // UpdateSessionDisplaySettings() throws exception if no login complete.
        return api().UpdateSessionDisplaySettings(desktop_width,
          desktop_height, physical_width, physical_height, orientation,
          desktop_scale_factor, device_scale_factor);
      } catch (...) {
        throw std::runtime_error{"cannot update RDP session display settings"};
      }
    }();
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot update RDP session display settings");
  }

  void sync_session_display_settings()
  {
    const auto err = api().SyncSessionDisplaySettings();
    DMITIGR_WINCOM_THROW_IF_ERROR(err,
      "cannot synchronize RDP session display settings");
  }

  // IMsRdpClient7

  template<class String>
  String status_text(const UINT status_code) const
  {
    return String(detail::api(*this).GetStatusText(status_code));
  }

  // IMsRdpExtendedSettings

  void set_property_disable_auto_reconnect_component(const bool value)
  {
    VARIANT val{};
    VariantInit(&val);
    val.vt = VT_BOOL;
    val.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    const auto err = api<MSTSCLib::IMsRdpExtendedSettings>()
      .put_Property(detail::bstr("DisableAutoReconnectComponent"), &val);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot disable auto reconnect component");
  }

private:
  Advise_sink_connection<Event_dispatcher> sink_;
};

} // namespace dmitigr::wincom::rdpts
