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

# This module defines the following extra variables:
#
# ${dmitigr_librarian_lib}_EXTRA_LIBRARIES - suggested extra libraries to link

set(dmitigr_librarian_lib Uv)
set(${dmitigr_librarian_lib}_include_names uv.h)
if(BUILD_SHARED_LIBS)
  set(${dmitigr_librarian_lib}_library_names uv libuv)
else()
  set(${dmitigr_librarian_lib}_library_names uv_a libuv_a uv libuv)
endif()
include(dmitigr_librarian)

if(NOT Uv_FOUND)
  return()
endif()

if(DEFINED uv_libraries)
  set(dmitigr_uv_libraries_stashed ${uv_libraries})
  unset(uv_libraries)
endif()
if(WIN32)
  list(APPEND uv_libraries
    psapi
    user32
    advapi32
    iphlpapi
    userenv
    ws2_32)
else()
  if(NOT CMAKE_SYSTEM_NAME MATCHES "Android|OS390|QNX")
    list(APPEND uv_libraries pthread)
  endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "AIX")
  list(APPEND uv_libraries perfstat)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  list(APPEND uv_libraries dl)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND uv_libraries dl rt)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  list(APPEND uv_libraries kvm)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "OS390")
  list(APPEND uv_libraries -Wl,xplink)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  list(APPEND uv_libraries kstat nsl sendfile socket)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
  list(APPEND uv_libraries bsd network)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
  list(APPEND uv_libraries socket)
endif()
set(${dmitigr_librarian_lib}_EXTRA_LIBRARIES ${uv_libraries})
if(DEFINED dmitigr_uv_libraries_stashed)
  set(uv_libraries ${dmitigr_uv_libraries_stashed})
  unset(dmitigr_uv_libraries_stashed)
endif()
