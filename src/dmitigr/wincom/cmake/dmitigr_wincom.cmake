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

dmitigr_libs_set_library_info(wincom 0 0 0 "Windows COM stuff")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

if (NOT WIN32)
  return()
endif()

set(dmitigr_wincom_headers
  enumerator.hpp
  exceptions.hpp
  firewall.hpp
  library.hpp
  object.hpp
  rdp.hpp
  tasc.hpp
  wmi.hpp
)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_wincom_deps base winbase)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  set(dmitigr_wincom_tests firewall)
  set(dmitigr_wincom_tests_target_link_libraries dmitigr_base)
endif()
