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

#pragma once

#include "../base/contract.hpp"
#include "../winbase/windows.hpp"
#include "enumerator.hpp"
#include "exceptions.hpp"
#include "object.hpp"

#include <comdef.h> // avoid LNK2019
#include <comutil.h>
#include <oleauto.h>
#include <Rdpencomapi.h>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace dmitigr::wincom::rdp {

class Invitation final : public
  Unknown_api<Invitation, IRDPSRAPIInvitation> {
  using Ua = Unknown_api<Invitation, IRDPSRAPIInvitation>;
public:
  using Ua::Ua;

  template<class String>
  String connection() const
  {
    return detail::get<String>(*this, &Api::get_ConnectionString);
  }

  bool is_revoked() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_Revoked(&result);
    return result == VARIANT_TRUE;
  }

  void revoke(const bool value = true)
  {
    const VARIANT_BOOL val{value ? VARIANT_TRUE : VARIANT_FALSE};
    api().put_Revoked(val);
  }
};

class Invitation_manager final : public
  Unknown_api<Invitation_manager, IRDPSRAPIInvitationManager> {
  using Ua = Unknown_api<Invitation_manager, IRDPSRAPIInvitationManager>;
public:
  using Ua::Ua;

  template<class GroupString, class PasswordString>
  Invitation create_invitation(const GroupString& group,
    const PasswordString& password, const long limit)
  {
    IRDPSRAPIInvitation* invitation{};
    const auto err = api().CreateInvitation(
      nullptr,
      detail::bstr(group),
      detail::bstr(password),
      limit,
      &invitation);
    if (err != S_OK)
      throw Win_error{"cannot create IRDPSRAPIInvitation instance", err};
    return Invitation{invitation};
  }

  long invitation_count() const
  {
    long result{};
    detail::api(*this).get_Count(&result);
    return result;
  }

  Invitation invitation(const long index) const
  {
    if (!(index < invitation_count()))
      throw std::out_of_range{"invitation index out of range"};

    IRDPSRAPIInvitation* raw{};
    VARIANT idx{};
    VariantInit(&idx);
    idx.vt = VT_I4;
    idx.lVal = index;
    detail::api(*this).get_Item(idx, &raw);
    check(raw, "invalid invitation retrieved from invitation manager");
    return Invitation{raw};
  }
};

class Tcp_connection_info final
  : public Unknown_api<Tcp_connection_info, IRDPSRAPITcpConnectionInfo> {
  using Ua = Unknown_api<Tcp_connection_info, IRDPSRAPITcpConnectionInfo>;
public:
  using Ua::Ua;

  template<class String>
  String local_address() const
  {
    return detail::get<String>(*this, &Api::get_LocalIP);
  }

  long local_port() const
  {
    long result{};
    detail::api(*this).get_LocalPort(&result);
    return result;
  }

  template<class String>
  String remote_address() const
  {
    return detail::get<String>(*this, &Api::get_PeerIP);
  }

  long remote_port() const
  {
    long result{};
    detail::api(*this).get_PeerPort(&result);
    return result;
  }

  long protocol() const
  {
    long result{};
    detail::api(*this).get_Protocol(&result);
    return result;
  }
};

class Attendee final : public
  Unknown_api<Attendee, IRDPSRAPIAttendee> {
  using Ua = Unknown_api<Attendee, IRDPSRAPIAttendee>;
public:
  using Ua::Ua;

  long id() const
  {
    long result{};
    detail::api(*this).get_Id(&result);
    return result;
  }

  Tcp_connection_info tcp_connection_info() const
  {
    IUnknown* info{};
    detail::api(*this).get_ConnectivityInfo(&info);
    check(info, "invalid connectivity info retrieved from attendee");
    try {
      return Tcp_connection_info::query(info);
    } catch (const std::runtime_error&) {}
    return Tcp_connection_info{};
  }

  void set_control_level(const CTRL_LEVEL level)
  {
    api().put_ControlLevel(level);
  }

  void terminate_connection()
  {
    api().TerminateConnection();
  }

  Invitation invitation() const
  {
    IRDPSRAPIInvitation* raw{};
    detail::api(*this).get_Invitation(&raw);
    check(raw, "invalid invitation retrieved from attendee");
    return Invitation{raw};
  }
};

class Attendee_manager final : public
  Unknown_api<Attendee_manager, IRDPSRAPIAttendeeManager> {
  using Ua = Unknown_api<Attendee_manager, IRDPSRAPIAttendeeManager>;
public:
  using Ua::Ua;

  Attendee attendee(const long id) const
  {
    IRDPSRAPIAttendee* raw{};
    detail::api(*this).get_Item(id, &raw);
    return Attendee{raw};
  }

  Enumerator<IEnumUnknown, IUnknown*> attendees() const
  {
    IUnknown* raw{};
    detail::api(*this).get__NewEnum(&raw);
    check(raw, "invalid enumerator retrieved from attendee manager");
    return Enumerator<IEnumUnknown, IUnknown*>{raw};
  }
};

class Attendee_disconnect_info final : public
  Unknown_api<Attendee_disconnect_info, IRDPSRAPIAttendeeDisconnectInfo> {
  using Ua = Unknown_api<Attendee_disconnect_info, IRDPSRAPIAttendeeDisconnectInfo>;
public:
  using Ua::Ua;

  Attendee attendee() const
  {
    IRDPSRAPIAttendee* raw{};
    detail::api(*this).get_Attendee(&raw);
    check(raw, "invalid attendee retrieved from attendee disconnect info");
    return Attendee{raw};
  }

  long code() const
  {
    long val{};
    detail::api(*this).get_Code(&val);
    return val;
  }

  ATTENDEE_DISCONNECT_REASON reason() const
  {
    ATTENDEE_DISCONNECT_REASON val{};
    detail::api(*this).get_Reason(&val);
    return val;
  }
};

class Session_properties final : public
  Unknown_api<Session_properties, IRDPSRAPISessionProperties> {
  using Ua = Unknown_api<Session_properties, IRDPSRAPISessionProperties>;
public:
  using Ua::Ua;

  Session_properties&
  set_clipboard_redirect_enabled(const bool value)
  {
    VARIANT val{};
    VariantInit(&val);
    val.vt = VT_BOOL;
    val.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    api().put_Property(_bstr_t{"EnableClipboardRedirect"}, val);
    return *this;
  }

  Session_properties&
  set_clipboard_redirect_callback(IRDPSRAPIClipboardUseEvents* const value)
  {
    VARIANT val{};
    VariantInit(&val);
    val.vt = VT_UNKNOWN;
    val.punkVal = value;
    api().put_Property(_bstr_t{"SetClipboardRedirectCallback"}, val);
    return *this;
  }
};

// -----------------------------------------------------------------------------

class Event_dispatcher : public Advise_sink<_IRDPSessionEvents> {};

template<class BasicComObject>
class Basic_rdp_peer : private Noncopymove {
public:
  virtual ~Basic_rdp_peer() = default;

  Basic_rdp_peer(std::unique_ptr<BasicComObject> com,
    std::unique_ptr<Event_dispatcher> sink)
    : com_{forward_or_throw<std::invalid_argument>(std::move(com),
      "Basic_rdp_peer(com)")}
    , sink_{com_->api(), std::move(sink), this}
  {}

  const BasicComObject& com() const noexcept
  {
    return *com_;
  }

  BasicComObject& com() noexcept
  {
    return const_cast<BasicComObject&>(
      static_cast<const Basic_rdp_peer*>(this)->com());
  }

private:
  std::unique_ptr<BasicComObject> com_;
  Advise_sink_connection<Event_dispatcher> sink_;
};

using Viewer = Basic_com_object<RDPViewer, IRDPSRAPIViewer>;
using Sharer = Basic_com_object<RDPSession, IRDPSRAPISharingSession>;

using Client_base = Basic_rdp_peer<Viewer>;
using Server_base = Basic_rdp_peer<Sharer>;

// -----------------------------------------------------------------------------
// Server
// -----------------------------------------------------------------------------

class Server final : public Server_base {
public:
  using Server_base::Server_base;

  void open()
  {
    if (!is_open_) {
      const auto err = com().api().Open();
      if (err != S_OK)
        throw Win_error{"cannot open RDP server", err};
      is_open_ = true;
    }
  }

  void close()
  {
    if (is_open_) {
      const auto err = com().api().Close();
      if (err != S_OK)
        throw Win_error{"cannot close RDP server", err};
      is_open_ = false;
    }
  }

  bool is_open() const noexcept
  {
    return is_open_;
  }

  Invitation_manager invitation_manager()
  {
    IRDPSRAPIInvitationManager* api{};
    com().api().get_Invitations(&api);
    check(api, "invalid IRDPSRAPIInvitationManager instance retrieved");
    return Invitation_manager{api};
  }

  Attendee_manager attendee_manager()
  {
    IRDPSRAPIAttendeeManager* api{};
    com().api().get_Attendees(&api);
    check(api, "invalid IRDPSRAPIAttendeeManager instance retrieved");
    return Attendee_manager{api};
  }

  Session_properties session_properties()
  {
    IRDPSRAPISessionProperties* api{};
    com().api().get_Properties(&api);
    check(api, "invalid IRDPSRAPISessionProperties instance retrieved");
    return Session_properties{api};
  }

  void pause()
  {
    const auto err = com().api().Pause();
    if (err != S_OK)
      throw Win_error{"cannot pause RDP server", err};
  }

  void resume()
  {
    const auto err = com().api().Resume();
    if (err != S_OK)
      throw Win_error{"cannot resume RDP server", err};
  }

private:
  bool is_open_{};
};

// -----------------------------------------------------------------------------
// Client
// -----------------------------------------------------------------------------

class Client final : public Client_base {
public:
  using Client_base::Client_base;

  ~Client() override
  {
    try {
      close();
    } catch (...) {}
  }

  template<class ConnStr, class NameStr, class PassStr>
  void open(const ConnStr& connection_string,
    const NameStr& name, const PassStr& password)
  {
    const auto err = com().api().Connect(
      detail::bstr(connection_string),
      detail::bstr(name),
      detail::bstr(password));
    if (err != S_OK)
      throw Win_error{"cannot open RDP client", err};
  }

  void close()
  {
    const auto err = com().api().Disconnect();
    if (err != S_OK)
      throw Win_error{"cannot close RDP client", err};
  }

  void set_control_level(const CTRL_LEVEL level)
  {
    const auto err = com().api().RequestControl(level);
    if (err != S_OK)
      throw Win_error{"cannot set control level of RDP client", err};
  }

  Session_properties session_properties()
  {
    IRDPSRAPISessionProperties* api{};
    com().api().get_Properties(&api);
    check(api, "invalid IRDPSRAPISessionProperties instance retrieved");
    return Session_properties{api};
  }

  void set_smart_sizing_enabled(const bool value)
  {
    const VARIANT_BOOL val{value ? VARIANT_TRUE : VARIANT_FALSE};
    com().api().put_SmartSizing(val);
  }

  bool is_smart_sizing_enabled() const noexcept
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(com()).get_SmartSizing(&result);
    return result == VARIANT_TRUE;
  }
};

} // namespace dmitigr::wincom::rdp
