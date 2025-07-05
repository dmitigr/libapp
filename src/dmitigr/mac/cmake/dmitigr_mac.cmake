# -*- cmake -*-
#
# Copyright 2025 Dmitry Igrishin
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

dmitigr_libs_set_library_info(mac 0 0 0 "macOS specific")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_mac_headers
  cf.hpp
  io.hpp
  )

set(dmitigr_mac_implementations
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_mac_deps base)

list(APPEND dmitigr_mac_target_link_libraries_interface
  "-framework CoreFoundation" "-framework IOKit")

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  if(APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES MSYS|MinGW|Cygwin)
    set(dmitigr_mac_tests cf io_platform_uuid)
    set(dmitigr_mac_tests_target_link_libraries dmitigr_base)
  endif()
endif()
