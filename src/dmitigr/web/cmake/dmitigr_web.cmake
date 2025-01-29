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

dmitigr_libs_set_library_info(web 0 0 0 "Web framework")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_web_headers
  config.hpp
  errc.hpp
  errctg.hpp
  exceptions.hpp
  http.hpp
  lisp.hpp
  rajson.hpp
  util.hpp
  wsjrpc.hpp
  )

set(dmitigr_web_implementations
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_web_deps base http jrpc lisp rajson str tpl url ws)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_web_tests http wsjrpc)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(dmitigr_web_test_http_target_compile_options -Wno-array-bounds)
  endif()
endif()
