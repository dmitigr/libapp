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

dmitigr_libs_set_library_info(nix 0 0 0 "Unix specific")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_nix_headers
  dl.hpp
  error.hpp
  ipc_pipe.hpp
  process.hpp
)

if(LINUX OR APPLE)
  list(APPEND dmitigr_nix_headers detach.hpp ifaddrs.hpp)
endif()
if(FREEBSD OR APPLE)
  list(APPEND dmitigr_nix_headers sysctl.hpp)
endif()


set(dmitigr_nix_implementations
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_nix_deps base)

list(APPEND dmitigr_nix_target_link_libraries_interface dl)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  if(UNIX AND NOT CMAKE_SYSTEM_NAME MATCHES MSYS|MinGW|Cygwin)
    set(dmitigr_nix_tests detach dl ifaddrs)
  endif()
  if(UNIX AND NOT LINUX)
    list(APPEND dmitigr_nix_tests sysctl)
  endif()
 if(LINUX)
    list(APPEND dmitigr_nix_tests pipe pipe2 slow_write)
  endif()

  add_library(dmitigr_nix_dl SHARED
    ${dmitigr_libs_subroot}/nix/test/nix-dl-lib.cpp)
endif()
