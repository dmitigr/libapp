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

#pragma once
// #pragma comment(lib, "FirewallAPI")
// #pragma comment(lib, "Hnetcfg")

#include "object.hpp"

#include <netfw.h>

namespace dmitigr::wincom::firewall {

class Authorized_application final : public
  Basic_com_object<NetFwAuthorizedApplication, INetFwAuthorizedApplication> {
  using Bco = Basic_com_object<NetFwAuthorizedApplication, INetFwAuthorizedApplication>;
public:
  using Bco::Bco;

  bool is_enabled() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_Enabled(&result);
    return result == VARIANT_TRUE;
  }

  Authorized_application& set_enabled(const bool value)
  {
    const VARIANT_BOOL val{value ? VARIANT_TRUE : VARIANT_FALSE};
    api().put_Enabled(val);
    return *this;
  }

  NET_FW_IP_VERSION ip_version() const
  {
    NET_FW_IP_VERSION result{};
    detail::api(*this).get_IpVersion(&result);
    return result;
  }

  Authorized_application& set_ip_version(const NET_FW_IP_VERSION value)
  {
    api().put_IpVersion(value);
    return *this;
  }

  template<class String>
  String name() const
  {
    return detail::get<String>(*this, &Api::get_Name);
  }

  template<class String>
  Authorized_application& set_name(const String& value)
  {
    api().put_Name(detail::bstr(value));
    return *this;
  }

  template<class String>
  String process_image_file_name() const
  {
    return detail::get<String>(*this, &Api::get_ProcessImageFileName);
  }

  template<class String>
  Authorized_application& set_process_image_file_name(const String& value)
  {
    api().put_ProcessImageFileName(detail::bstr(value));
    return *this;
  }
};

// -----------------------------------------------------------------------------

class Authorized_applications final : public
  Unknown_api<Authorized_applications, INetFwAuthorizedApplications> {
  using Ua = Unknown_api<Authorized_applications, INetFwAuthorizedApplications>;
public:
  using Ua::Ua;

  Authorized_applications& add(const Authorized_application& app)
  {
    const auto err = api().Add(&detail::api(app));
    if (err != S_OK)
      throw Win_error{"cannot add application to firewall collection", err};
    return *this;
  }

  template<class String>
  Authorized_application& remove(const String& image_file_name)
  {
    const auto err = api().Remove(detail::bstr(image_file_name));
    if (err != S_OK)
      throw Win_error{"cannot add application to firewall collection", err};
    return *this;
  }
};

// -----------------------------------------------------------------------------

class Profile final : public Unknown_api<Profile, INetFwProfile> {
  using Ua = Unknown_api<Profile, INetFwProfile>;
public:
  using Ua::Ua;

  Authorized_applications authorized_applications() const
  {
    INetFwAuthorizedApplications* apps{};
    detail::api(*this).get_AuthorizedApplications(&apps);
    return Authorized_applications{apps};
  }
};

// -----------------------------------------------------------------------------

class Policy final : public Unknown_api<Policy, INetFwPolicy> {
  using Ua = Unknown_api<Policy, INetFwPolicy>;
public:
  using Ua::Ua;

  Profile current_profile() const
  {
    INetFwProfile* profile{};
    detail::api(*this).get_CurrentProfile(&profile);
    return Profile{profile};
  }

  Profile profile(const NET_FW_PROFILE_TYPE value) const
  {
    INetFwProfile* profile{};
    detail::api(*this).GetProfileByType(value, &profile);
    return Profile{profile};
  }
};

// -----------------------------------------------------------------------------

class Manager final : public Basic_com_object<NetFwMgr, INetFwMgr> {
  using Bco = Basic_com_object<NetFwMgr, INetFwMgr>;
public:
  using Bco::Bco;

  NET_FW_PROFILE_TYPE current_profile_type() const
  {
    NET_FW_PROFILE_TYPE result{};
    detail::api(*this).get_CurrentProfileType(&result);
    return result;
  }

  Policy local_policy() const
  {
    INetFwPolicy* policy{};
    detail::api(*this).get_LocalPolicy(&policy);
    return Policy{policy};
  }
};

// -----------------------------------------------------------------------------

class Rule final : public Basic_com_object<NetFwRule, INetFwRule> {
  using Bco = Basic_com_object<NetFwRule, INetFwRule>;
public:
  using Bco::Bco;

  template<class String>
  String name() const
  {
    return detail::get<String>(*this, &Api::get_Name);
  }

  template<class String>
  Rule& set_name(const String& value)
  {
    api().put_Name(detail::bstr(value));
    return *this;
  }

  template<class String>
  String application_name() const
  {
    return detail::get<String>(*this, &Api::get_ApplicationName);
  }

  template<class String>
  Rule& set_application_name(const String& image_file_name)
  {
    api().put_ApplicationName(detail::bstr(image_file_name));
    return *this;
  }

  template<class String>
  String description() const
  {
    return detail::get<String>(*this, &Api::get_Description);
  }

  template<class String>
  Rule& set_description(const String& value)
  {
    api().put_Description(detail::bstr(value));
    return *this;
  }

  template<class String>
  String grouping() const
  {
    return detail::get<String>(*this, &Api::get_Grouping);
  }

  template<class String>
  Rule& set_grouping(const String& context)
  {
    api().put_Grouping(detail::bstr(context));
    return *this;
  }

  template<class String>
  String interface_types() const
  {
    return detail::get<String>(*this, &Api::get_InterfaceTypes);
  }

  template<class String>
  Rule& set_interface_types(const String& value)
  {
    api().put_InterfaceTypes(detail::bstr(value));
    return *this;
  }

  template<class String>
  String remote_addresses() const
  {
    return detail::get<String>(*this, &Api::get_RemoteAddresses);
  }

  template<class String>
  Rule& set_remote_addresses(const String& value)
  {
    api().put_RemoteAddresses(detail::bstr(value));
    return *this;
  }

  template<class String>
  String remote_ports() const
  {
    return detail::get<String>(*this, &Api::get_RemotePorts);
  }

  template<class String>
  Rule& set_remote_ports(const String& value)
  {
    api().put_RemotePorts(detail::bstr(value));
    return *this;
  }

  long profiles() const
  {
    long result{};
    detail::api(*this).get_Profiles(&result);
    return result;
  }

  Rule& set_profiles(const long value)
  {
    api().put_Profiles(value);
    return *this;
  }

  long protocol() const
  {
    long result{};
    detail::api(*this).get_Protocol(&result);
    return result;
  }

  Rule& set_protocol(const long value)
  {
    api().put_Protocol(value);
    return *this;
  }

  bool is_enabled() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_Enabled(&result);
    return result == VARIANT_TRUE;
  }

  Rule& set_enabled(const bool value)
  {
    const VARIANT_BOOL val{value ? VARIANT_TRUE : VARIANT_FALSE};
    api().put_Enabled(val);
    return *this;
  }
};

// -----------------------------------------------------------------------------

class Rules final : public Unknown_api<Rules, INetFwRules> {
  using Ua = Unknown_api<Rules, INetFwRules>;
public:
  using Ua::Ua;

  Rules& add(const Rule& rule)
  {
    const auto err = api().Add(&detail::api(rule));
    if (err != S_OK)
      throw Win_error{"cannot add firewall rule", err};
    return *this;
  }

  template<class String>
  Rules& remove(const String& name)
  {
    const auto err = api().Remove(detail::bstr(name));
    if (err != S_OK)
      throw Win_error{"cannot remove firewall rule", err};
    return *this;
  }

  long count() const
  {
    long result{};
    detail::api(*this).get_Count(&result);
    return result;
  }

  template<class String>
  Rule rule(const String& name) const
  {
    INetFwRule* rul{};
    const auto err = detail::api(*this).Item(detail::bstr(name), &rul);
    if (err != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && err != S_OK)
      throw Win_error{"cannot retrieve firewall rule", err};
    return Rule{rul};
  }
};

// -----------------------------------------------------------------------------

class Policy2 final : public Basic_com_object<NetFwPolicy2, INetFwPolicy2> {
  using Bco = Basic_com_object<NetFwPolicy2, INetFwPolicy2>;
public:
  using Bco::Bco;

  /**
   * @param profiles A bitmask from NET_FW_PROFILE_TYPE2.
   */
  template<typename String>
  void enable_rule_group(const long profiles, const String& group,
    const bool is_enabled)
  {
    const VARIANT_BOOL enable{is_enabled ? VARIANT_TRUE : VARIANT_FALSE};
    const auto err = api().EnableRuleGroup(profiles, detail::bstr(group), enable);
    if (err != S_OK)
      throw Win_error{L"cannot toggle specified group of firewall rules", err};
  }

  template<typename String>
  bool is_rule_group_currently_enabled(const String& group) const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    const auto err = detail::api(*this)
      .get_IsRuleGroupCurrentlyEnabled(detail::bstr(group), &result);
    if (err != S_OK)
      throw Win_error{"cannot get firewall rule group status of current profile", err};
    return result == VARIANT_TRUE;
  }

  template<typename String>
  bool is_rule_group_enabled(const long profile, const String& group) const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    const auto err = detail::api(*this)
      .IsRuleGroupEnabled(profile, detail::bstr(group), &result);
    if (err != S_OK)
      throw Win_error{"cannot get firewall rule group status", err};
    return result == VARIANT_TRUE;
  }

  long current_profile_types() const
  {
    long result{};
    detail::api(*this).get_CurrentProfileTypes(&result);
    return result;
  }

  bool is_firewall_enabled(const NET_FW_PROFILE_TYPE2 profile) const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_FirewallEnabled(profile, &result);
    return result == VARIANT_TRUE;
  }

  NET_FW_MODIFY_STATE local_policy_modify_state() const
  {
    NET_FW_MODIFY_STATE result{};
    detail::api(*this).get_LocalPolicyModifyState(&result);
    return result;
  }

  Rules rules() const
  {
    INetFwRules* rules{};
    detail::api(*this).get_Rules(&rules);
    return Rules{rules};
  }
};

} // namespace dmitigr::wincom::firewall
