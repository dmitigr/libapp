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

dmitigr_libs_set_library_info(rajson 0 0 0 "RapidJSON wrapper")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_rajson_headers
  conversions.hpp
  document.hpp
  emplace.hpp
  errc.hpp
  errctg.hpp
  exceptions.hpp
  fwd.hpp
  path.hpp
  value_view.hpp
  )

set(dmitigr_rajson_implementations
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_rajson_deps base 3rdparty_rapidjson)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_rajson_tests conversions document emplace value_view)
  set(dmitigr_rajson_tests_target_link_libraries dmitigr_base dmitigr_str)

  set(prefix ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/rajson/test)
  add_custom_target(dmitigr_rajson_copy_test_resources ALL
    COMMAND cmake -E copy_if_different
    "${prefix}/rajson-unit-value_view.json"
    "${dmitigr_libs_resource_destination_dir}"
    )
  add_dependencies(dmitigr_rajson_copy_test_resources
    dmitigr_libs_create_resource_destination_dir)
endif()
