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

#include "../base/assert.hpp"
#include "../base/utility.hpp"
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
#include <optional>
#include <stdexcept>

namespace dmitigr::wincom::rdp {

inline bool is_view(const CTRL_LEVEL cl) noexcept
{
  return cl == CTRL_LEVEL_VIEW || cl == CTRL_LEVEL_REQCTRL_VIEW;
}

inline bool is_interactive(const CTRL_LEVEL cl) noexcept
{
  return cl == CTRL_LEVEL_INTERACTIVE || cl == CTRL_LEVEL_REQCTRL_INTERACTIVE;
}

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

  RDPENCOMAPI_ATTENDEE_FLAGS flags() const
  {
    long result{};
    detail::api(*this).get_Flags(&result);
    return static_cast<RDPENCOMAPI_ATTENDEE_FLAGS>(result);
  }

  bool is_local() const
  {
    return flags() == ATTENDEE_FLAGS_LOCAL;
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

  CTRL_LEVEL control_level() const
  {
    CTRL_LEVEL result{};
    detail::api(*this).get_ControlLevel(&result);
    return result;
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

  /**
   * @returns The enumerator of variants of type `VT_DISPATCH`.
   *
   * @remarks Attendee instance can be queried from each element of
   * the returned enumerator as `Attendee::query(element.pdispVal)`.
   */
  Enumerator<IEnumVARIANT, VARIANT> attendees() const
  {
    IUnknown* raw{};
    detail::api(*this).get__NewEnum(&raw);
    check(raw, "invalid enumerator retrieved from attendee manager");
    return Enumerator<IEnumVARIANT, VARIANT>{raw};
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

  Session_properties&
  set_default_attendee_control_level(const CTRL_LEVEL value)
  {
    VARIANT val{};
    VariantInit(&val);
    val.vt = VT_I4;
    val.lVal = value;
    api().put_Property(_bstr_t{"DefaultAttendeeControlLevel"}, val);
    return *this;
  }

  CTRL_LEVEL default_attendee_control_level() const
  {
    VARIANT val{};
    VariantInit(&val);
    detail::api(*this).get_Property(_bstr_t{"DefaultAttendeeControlLevel"}, &val);
    return static_cast<CTRL_LEVEL>(val.lVal);
  }
};

// -----------------------------------------------------------------------------

class Event_dispatcher : public Advise_sink<_IRDPSessionEvents> {};

template<class BasicComObject>
class Basic_rdp_peer : private Noncopymove {
public:
  using Com = BasicComObject;

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

class Client;

namespace detail {
class Viewer_event_dispatcher final : public Event_dispatcher {
public:
  explicit Viewer_event_dispatcher(std::unique_ptr<Event_dispatcher> rep)
    : rep_{std::move(rep)}
  {}

  void set_owner(void*) override;
  HRESULT Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*,
    UINT*) override;

private:
  std::unique_ptr<Event_dispatcher> rep_;
  Client* client_{};
};
} // namespace detail

class Client final : public Client_base {
public:
  explicit Client(std::unique_ptr<Viewer> com)
    : Client{std::move(com), nullptr}
  {}

  Client(std::unique_ptr<Viewer> com, std::unique_ptr<Event_dispatcher> sink)
    : Client_base{std::move(com),
      std::make_unique<detail::Viewer_event_dispatcher>(std::move(sink))}
  {}

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
    using wincom::detail::bstr;
    const auto err = com().api().Connect(bstr(connection_string),
      bstr(name), bstr(password));
    if (err != S_OK)
      throw Win_error{"cannot open RDP client", err};
    is_open_ = true;
  }

  bool is_open() const
  {
    return is_open_;
  }

  void close()
  {
    if (is_open_) {
      const auto err = com().api().Disconnect();
      if (err != S_OK)
        throw Win_error{"cannot close RDP client", err};
      is_open_ = false;
    }
  }

  /**
   * @see Attendee::control_level().
   */
  void set_control_level(const CTRL_LEVEL level)
  {
    const auto err = com().api().RequestControl(level);
    if (err != S_OK)
      throw Win_error{"cannot set control level of RDP client", err};
  }

  Attendee_manager attendee_manager() const
  {
    IRDPSRAPIAttendeeManager* api{};
    wincom::detail::api(com()).get_Attendees(&api);
    check(api, "invalid IRDPSRAPIAttendeeManager instance retrieved");
    return Attendee_manager{api};
  }

  /**
   * @returns The attendee of this instance (local attendee), or invalid
   * instance if not authenticated.
   */
  Attendee attendee() const
  {
    if (!attendee_id_) {
      auto atts = attendee_manager().attendees();
      while (auto att = atts.next()) {
        if (auto a = rdp::Attendee::query(att->pdispVal); a.is_local()) {
          attendee_id_ = a.id();
          return a;
        }
      }
    } else
      return attendee_manager().attendee(*attendee_id_);
    return Attendee{};
  }

  Session_properties session_properties() const
  {
    IRDPSRAPISessionProperties* api{};
    wincom::detail::api(com()).get_Properties(&api);
    check(api, "invalid IRDPSRAPISessionProperties instance retrieved");
    return Session_properties{api};
  }

  void set_smart_sizing_enabled(const bool value)
  {
    const VARIANT_BOOL val{value ? VARIANT_TRUE : VARIANT_FALSE};
    com().api().put_SmartSizing(val);
  }

  bool is_smart_sizing_enabled() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    wincom::detail::api(com()).get_SmartSizing(&result);
    return result == VARIANT_TRUE;
  }

private:
  friend detail::Viewer_event_dispatcher;
  bool is_open_{};
  mutable std::optional<long> attendee_id_;
};

namespace detail {
void Viewer_event_dispatcher::set_owner(void* const owner)
{
  DMITIGR_ASSERT(owner);
  client_ = static_cast<Client*>(owner);
  if (rep_)
    rep_->set_owner(owner);
}

HRESULT Viewer_event_dispatcher::Invoke(const DISPID disp_id,
  REFIID riid,
  const LCID cid,
  const WORD flags,
  DISPPARAMS* const disp_params,
  VARIANT* const result,
  EXCEPINFO* const excep_info,
  UINT* const arg_err)
{
  UNREFERENCED_PARAMETER(riid);
  UNREFERENCED_PARAMETER(cid);
  UNREFERENCED_PARAMETER(flags);
  UNREFERENCED_PARAMETER(disp_params);
  UNREFERENCED_PARAMETER(result);
  UNREFERENCED_PARAMETER(excep_info);
  UNREFERENCED_PARAMETER(arg_err);
  switch (disp_id) {
  case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_CONNECTED: {
    IDispatch* const dispatcher{disp_params->rgvarg[0].pdispVal};
    auto attendee = Attendee::query(dispatcher);
    break;
  }
  case DISPID_RDPSRAPI_EVENT_ON_ERROR:
    [[fallthrough]];
  case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTED:
    [[fallthrough]];
  case DISPID_RDPSRAPI_EVENT_ON_VIEWER_DISCONNECTED:
    [[fallthrough]];
  case DISPID_RDPSRAPI_EVENT_ON_VIEWER_AUTHENTICATED:
    [[fallthrough]];
  case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTFAILED:
    [[fallthrough]];
  default:
    break;
  }
  return rep_ ? rep_->Invoke(disp_id, riid, cid, flags, disp_params, result,
    excep_info, arg_err) : S_OK;
}
} // namespace detail

} // namespace dmitigr::wincom::rdp
