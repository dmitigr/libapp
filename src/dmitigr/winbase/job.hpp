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
#pragma comment(lib, "kernel32")

#include "error.hpp"
#include "hguard.hpp"

#include <stdexcept>
#include <utility>

#include <jobapi2.h>

namespace dmitigr::winbase {

/// A job.
class Job final {
public:
  Job() = default;

  HANDLE handle() const noexcept
  {
    return job_.handle();
  }

  operator HANDLE() const noexcept
  {
    return handle();
  }

  void assign_process(const HANDLE process)
  {
    if (!AssignProcessToJobObject(job_, process))
      throw std::runtime_error{last_error_message()};
  }

private:
  friend Job make_process_termination_job();

  Handle_guard job_;

  explicit Job(Handle_guard job)
    : job_{std::move(job)}
  {}
};

/**
 * @returns The created job object which terminates all associated processes
 * when goes out of the scope.
 */
inline Job make_process_termination_job()
{
  Job result{Handle_guard{CreateJobObjectW(nullptr, nullptr)}};
  if (!result)
    throw std::runtime_error{last_error_message()};

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
  info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(result, JobObjectExtendedLimitInformation,
      &info, sizeof(info)))
    throw std::runtime_error{last_error_message()};

  return result;
}

} // namespace dmitigr::winbase
