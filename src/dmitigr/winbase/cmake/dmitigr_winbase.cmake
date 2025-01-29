# -*- cmake -*-
#
# Copyright 2023 Dmitry Igrishin
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ------------------------------------------------------------------------------
# Info
# ------------------------------------------------------------------------------

dmitigr_libs_set_library_info(winbase 0 0 0 "Windows base stuff")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

if (NOT WIN32)
  return()
endif()

set(dmitigr_winbase_headers
  account.hpp
  combase.hpp
  dialog.hpp
  error.hpp
  exceptions.hpp
  hguard.hpp
  hlocal.hpp
  ipc_wm.hpp
  iphelper.hpp
  job.hpp
  menu.hpp
  netman.hpp
  processenv.hpp
  process.hpp
  program.hpp
  registry.hpp
  resource.hpp
  security.hpp
  shell.hpp
  strconv.hpp
  sync.hpp
  sysinfo.hpp
  userenv.hpp
  windows.hpp
  winsta.hpp
  wow64.hpp
  wts.hpp
)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_winbase_deps base str)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  set(dmitigr_winbase_tests account netman safearray wts)
  set(dmitigr_winbase_tests_target_link_libraries dmitigr_base)
endif()
