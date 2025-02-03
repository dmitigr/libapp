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

dmitigr_libs_set_library_info(os 0 0 0 "OS facilities")

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_os_deps base)

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_os_headers
  basics.hpp
  environment.hpp
  error.hpp
  exceptions.hpp
  last_error.hpp
  pid.hpp
  types_fwd.hpp
  windows.hpp
)

if(NOT APPLE)
  list(APPEND dmitigr_os_headers smbios.hpp)
endif()

set(dmitigr_os_implementations
  )

if(WIN32)
  list(APPEND dmitigr_os_headers windows.hpp)
endif()

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_os_deps base)
if(WIN32)
  list(APPEND dmitigr_libs_os_deps winbase)
elseif(UNIX)
  list(APPEND dmitigr_libs_os_deps nix)
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_os_tests)
  if(NOT APPLE)
    list(APPEND dmitigr_os_tests smbios)
  endif()
  set(dmitigr_os_tests_target_link_libraries dmitigr_base)
endif()
