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

dmitigr_libs_set_library_info(wscl 1 0 0 "WebSocket client library")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_wscl_headers
  basics.hpp
  connection.hpp
  connection_options.hpp
  exceptions.hpp
  )

set(dmitigr_wscl_implementations
  connection.cpp
  connection_options.cpp
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_wscl_deps base net 3rdparty_uwsc)

if (NOT DMITIGR_LIBS_HEADER_ONLY)
  set(suffix "public")
else()
  set(suffix "interface")
endif()

# dmitigr_3rdparty_uwsc must be exported anyway. If header-only mode is off,
# it will be exported as interface. Otherwise, it will be exported as static
# library. So, target_link_libraries cannot be private in this case.
set(dmitigr_wscl_target_link_libraries_${suffix} dmitigr_3rdparty_uwsc)

if(WIN32)
  set(dmitigr_wscl_target_compile_definitions_${suffix} WIN32_LEAN_AND_MEAN)
endif()

if(DMITIGR_LIBS_OPENSSL)
  set(dmitigr_wscl_target_compile_definitions_private DMITIGR_LIBS_OPENSSL)
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_wscl_tests hello jrpc)
  set(dmitigr_wscl_tests_target_link_libraries dmitigr_base dmitigr_uv)
  if(WIN32)
    set(dmitigr_wscl_tests_target_compile_definitions WIN32_LEAN_AND_MEAN)
  endif()
endif()
