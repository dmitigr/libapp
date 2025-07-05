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
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "wbemuuid")

#include "../base/noncopymove.hpp"
#include "../winbase/combase.hpp"
#include "exceptions.hpp"
#include "object.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

#include <taskschd.h>

namespace dmitigr::wincom::tasc::v2 {

inline const char* to_literal(const TASK_STATE value) noexcept
{
  switch (value) {
  case TASK_STATE_UNKNOWN:
    return "TASK_STATE_UNKNOWN";
  case TASK_STATE_DISABLED:
    return "TASK_STATE_DISABLED";
  case TASK_STATE_QUEUED:
    return "TASK_STATE_QUEUED";
  case TASK_STATE_READY:
    return "TASK_STATE_READY";
  case TASK_STATE_RUNNING:
    return "TASK_STATE_RUNNING";
  }
  return nullptr;
}

inline const char* to_literal(const TASK_TRIGGER_TYPE2 value) noexcept
{
  switch (value) {
  case TASK_TRIGGER_EVENT:
    return "TASK_TRIGGER_EVENT";
  case TASK_TRIGGER_TIME:
    return "TASK_TRIGGER_TIME";
  case TASK_TRIGGER_DAILY:
    return "TASK_TRIGGER_DAILY";
  case TASK_TRIGGER_WEEKLY:
    return "TASK_TRIGGER_WEEKLY";
  case TASK_TRIGGER_MONTHLY:
    return "TASK_TRIGGER_MONTHLY";
  case TASK_TRIGGER_MONTHLYDOW:
    return "TASK_TRIGGER_MONTHLYDOW";
  case TASK_TRIGGER_IDLE:
    return "TASK_TRIGGER_IDLE";
  case TASK_TRIGGER_REGISTRATION:
    return "TASK_TRIGGER_REGISTRATION";
  case TASK_TRIGGER_BOOT:
    return "TASK_TRIGGER_BOOT";
  case TASK_TRIGGER_LOGON:
    return "TASK_TRIGGER_LOGON";
  case TASK_TRIGGER_SESSION_STATE_CHANGE:
    return "TASK_TRIGGER_SESSION_STATE_CHANGE";
  case TASK_TRIGGER_CUSTOM_TRIGGER_01:
    return "TASK_TRIGGER_CUSTOM_TRIGGER_01";
  }
  return nullptr;
}

class Registration_info final :
  public Unknown_api<Registration_info, IRegistrationInfo> {
  using Ua = Unknown_api<Registration_info, IRegistrationInfo>;
public:
  using Ua::Ua;

  template<class String>
  String author() const
  {
    return detail::get<String>(*this, &Api::get_Author);
  }

  template<class String>
  String date() const
  {
    return detail::get<String>(*this, &Api::get_Date);
  }

  template<class String>
  String description() const
  {
    return detail::get<String>(*this, &Api::get_Description);
  }

  template<class String>
  String documentation() const
  {
    return detail::get<String>(*this, &Api::get_Documentation);
  }

  template<class String>
  String source() const
  {
    return detail::get<String>(*this, &Api::get_Source);
  }

  template<class String>
  String uri() const
  {
    return detail::get<String>(*this, &Api::get_URI);
  }

  template<class String>
  String version() const
  {
    return detail::get<String>(*this, &Api::get_Version);
  }

  template<class String>
  String xml_text() const
  {
    return detail::get<String>(*this, &Api::get_XmlText);
  }
};

class Repetition_pattern final :
  public Unknown_api<Repetition_pattern, IRepetitionPattern> {
  using Ua = Unknown_api<Repetition_pattern, IRepetitionPattern>;
public:
  using Ua::Ua;

  template<class String>
  String duration() const
  {
    return detail::get<String>(*this, &Api::get_Duration);
  }

  template<class String>
  String interval() const
  {
    return detail::get<String>(*this, &Api::get_Interval);
  }

  bool is_stopped_at_the_end_of_duration() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_StopAtDurationEnd(&result);
    return result == VARIANT_TRUE;
  }
};

class Trigger final :
  public Unknown_api<Trigger, ITrigger> {
  using Ua = Unknown_api<Trigger, ITrigger>;
public:
  using Ua::Ua;

  TASK_TRIGGER_TYPE2 type() const
  {
    TASK_TRIGGER_TYPE2 result{};
    detail::api(*this).get_Type(&result);
    return result;
  }

  bool is_enabled() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_Enabled(&result);
    return result == VARIANT_TRUE;
  }

  template<class String>
  String id() const
  {
    return detail::get<String>(*this, &Api::get_Id);
  }

  template<class String>
  String start_boundary() const
  {
    return detail::get<String>(*this, &Api::get_StartBoundary);
  }

  template<class String>
  String end_boundary() const
  {
    return detail::get<String>(*this, &Api::get_EndBoundary);
  }

  template<class String>
  String execution_time_limit() const
  {
    return detail::get<String>(*this, &Api::get_ExecutionTimeLimit);
  }

  Repetition_pattern repetition_pattern() const
  {
    IRepetitionPattern* result{};
    detail::api(*this).get_Repetition(&result);
    return Repetition_pattern{result};
  }
};

class Trigger_collection final :
  public Unknown_api<Trigger_collection, ITriggerCollection> {
  using Ua = Unknown_api<Trigger_collection, ITriggerCollection>;
public:
  using Ua::Ua;

  LONG count() const
  {
    LONG result{};
    detail::api(*this).get_Count(&result);
    return result;
  }

  /**
   * @param index 1-based index.
   */
  Trigger item(const LONG index) const
  {
    ITrigger* result{};
    detail::api(*this).get_Item(index, &result);
    return Trigger{result};
  }
};

class Task_definition final :
  public Unknown_api<Task_definition, ITaskDefinition> {
  using Ua = Unknown_api<Task_definition, ITaskDefinition>;
public:
  using Ua::Ua;

  Trigger_collection triggers() const
  {
    ITriggerCollection* result{};
    detail::api(*this).get_Triggers(&result);
    return Trigger_collection{result};
  }

  Registration_info registration_info() const
  {
    IRegistrationInfo* result{};
    detail::api(*this).get_RegistrationInfo(&result);
    return Registration_info{result};
  }
};

class Registered_task final :
  public Unknown_api<Registered_task, IRegisteredTask> {
  using Ua = Unknown_api<Registered_task, IRegisteredTask>;
public:
  using Ua::Ua;

  template<class String>
  String name() const
  {
    return detail::get<String>(*this, &Api::get_Name);
  }

  template<class String>
  String path() const
  {
    return detail::get<String>(*this, &Api::get_Path);
  }

  TASK_STATE state() const
  {
    TASK_STATE result{};
    detail::api(*this).get_State(&result);
    return result;
  }

  DATE last_run_time() const
  {
    DATE result{};
    detail::api(*this).get_LastRunTime(&result);
    return result;
  }

  DATE next_run_time() const
  {
    DATE result{};
    detail::api(*this).get_NextRunTime(&result);
    return result;
  }

  Task_definition task_definition() const
  {
    ITaskDefinition* result{};
    detail::api(*this).get_Definition(&result);
    return Task_definition{result};
  }
};

class Registered_task_collection final :
  public Unknown_api<Registered_task_collection, IRegisteredTaskCollection> {
  using Ua = Unknown_api<Registered_task_collection, IRegisteredTaskCollection>;
public:
  using Ua::Ua;

  LONG count() const
  {
    LONG result{};
    detail::api(*this).get_Count(&result);
    return result;
  }

  /**
   * @param index 1-based index.
   */
  Registered_task item(const LONG index) const
  {
    IRegisteredTask* result{};
    detail::api(*this).get_Item(_variant_t(index), &result);
    return Registered_task{result};
  }
};

class Task_folder;

class Task_folder_collection final :
  public Unknown_api<Task_folder_collection, ITaskFolderCollection> {
  using Ua = Unknown_api<Task_folder_collection, ITaskFolderCollection>;
public:
  using Ua::Ua;

  LONG count() const
  {
    LONG result{};
    detail::api(*this).get_Count(&result);
    return result;
  }

  /**
   * @param index 1-based index.
   */
  Task_folder item(LONG index) const;
};

class Task_folder final :
  public Unknown_api<Task_folder, ITaskFolder> {
  using Ua = Unknown_api<Task_folder, ITaskFolder>;
public:
  using Ua::Ua;

  Task_folder_collection folders() const
  {
    ITaskFolderCollection* result{};
    const auto err = detail::api(*this).GetFolders(0, &result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get all the subfolders in the"
      " folder of registered tasks");
    return Task_folder_collection{result};
  }

  /**
   * @params flags Specifies whether to retrieve hidden tasks. Value of
   * `TASK_ENUM_HIDDEN` should be used to retrieve all tasks in the folder
   * including hidden tasks. Value of `0` should be used to retrieve all
   * the tasks in the folder excluding the hidden tasks.
   */
  Registered_task_collection tasks(const LONG flags) const
  {
    IRegisteredTaskCollection* result{};
    const auto err = detail::api(*this).GetTasks(flags, &result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get all the tasks in the folder");
    return Registered_task_collection{result};
  }
};

Task_folder Task_folder_collection::item(const LONG index) const
{
  ITaskFolder* result{};
  detail::api(*this).get_Item(_variant_t(index), &result);
  return Task_folder{result};
}

class Task_service final : public Basic_com_object<TaskScheduler, ITaskService> {
  using Bco = Basic_com_object<TaskScheduler, ITaskService>;
public:
  using Bco::Bco;

  void connect(const winbase::com::Const_variant& server_name = {},
    const winbase::com::Const_variant& user = {},
    const winbase::com::Const_variant& domain = {},
    const winbase::com::Const_variant& password = {})
  {
    const auto err = api().Connect(
      server_name.data(),
      user.data(),
      domain.data(),
      password.data());
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot connect to remote computer and"
      " associate all subsequent calls on "+std::string{"ITaskService"}
      +" interface with a local (remote) session");
  }

  bool is_connected() const
  {
    VARIANT_BOOL result{VARIANT_FALSE};
    detail::api(*this).get_Connected(&result);
    return result == VARIANT_TRUE;
  }

  template<class String>
  Task_folder folder(const String& path) const
  {
    ITaskFolder* result{};
    const auto err = detail::api(*this).GetFolder(_bstr_t(path.c_str()), &result);
    DMITIGR_WINCOM_THROW_IF_ERROR(err, "cannot get folder of registered tasks");
    return Task_folder{result};
  }
};

} // namespace dmitigr::wincom::tasc::v2
