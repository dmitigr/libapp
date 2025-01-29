# -*- cmake -*-
#
# Copyright 2024 Dmitry Igrishin
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

dmitigr_libs_set_library_info(sqlixx 1 0 3 "SQLite API")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_sqlixx_headers
  connection.hpp
  conversions.hpp
  data.hpp
  errctg.hpp
  exceptions.hpp
  statement.hpp
)

set(dmitigr_sqlixx_implementations
)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_sqlixx_deps base)

if(DMITIGR_LIBS_SQLITE3)
  find_package(SQLite3 REQUIRED)
  list(APPEND dmitigr_sqlixx_target_include_directories_interface "${SQLite3_INCLUDE_DIRS}")
  list(APPEND dmitigr_sqlixx_target_link_libraries_interface ${SQLite3_LIBRARIES})
endif()

if (UNIX)
  list(APPEND dmitigr_sqlixx_target_link_libraries_interface pthread)
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_sqlixx_tests test)
  list(APPEND dmitigr_sqlixx_tests_target_link_libraries dmitigr_base)
endif()
