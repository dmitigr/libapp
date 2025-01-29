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

dmitigr_libs_set_library_info(http 0 0 0 "HTTP library")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_http_headers
  basics.hpp
  client.hpp
  connection.hpp
  cookie.hpp
  date.hpp
  errc.hpp
  errctg.hpp
  exceptions.hpp
  header.hpp
  server.hpp
  set_cookie.hpp
  syntax.hpp
  types_fwd.hpp
  )

set(dmitigr_http_implementations
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_http_deps base dt net str)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_http_tests basics cookie date set_cookie server client)
  set(dmitigr_http_tests_target_link_libraries dmitigr_base dmitigr_dt)
endif()
