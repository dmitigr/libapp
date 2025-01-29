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

dmitigr_libs_set_library_info(uv 0 0 0 "libuv wrapper")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_uv_headers
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_uv_deps base)

find_package(Uv REQUIRED)
list(APPEND dmitigr_uv_target_include_directories_public "${Uv_INCLUDE_DIRS}")
list(APPEND dmitigr_uv_target_include_directories_interface "${Uv_INCLUDE_DIRS}")
list(APPEND dmitigr_uv_target_link_libraries_public ${Uv_LIBRARIES}
  ${Uv_EXTRA_LIBRARIES})
list(APPEND dmitigr_uv_target_link_libraries_interface ${Uv_LIBRARIES}
  ${Uv_EXTRA_LIBRARIES})

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_uv_tests)
endif()
