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

dmitigr_libs_set_library_info(mulf 0 0 0 "Multipart form data library")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_mulf_headers
  exceptions.hpp
  form_data.hpp
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_mulf_deps base)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_mulf_tests valid)
  set(dmitigr_mulf_tests_target_link_libraries dmitigr_base)

  set(prefix ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/mulf/test)
  add_custom_target(dmitigr_mulf_copy_test_resources ALL
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-valid1.txt"
    "${dmitigr_libs_resource_destination_dir}"
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-invalid1.txt"
    "${dmitigr_libs_resource_destination_dir}"
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-invalid2.txt"
    "${dmitigr_libs_resource_destination_dir}"
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-invalid3.txt"
    "${dmitigr_libs_resource_destination_dir}"
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-invalid4.txt"
    "${dmitigr_libs_resource_destination_dir}"
    COMMAND cmake -E copy_if_different
    "${prefix}/mulf-form-data-invalid5.txt"
    "${dmitigr_libs_resource_destination_dir}"
    )
  add_dependencies(dmitigr_mulf_copy_test_resources
    dmitigr_libs_create_resource_destination_dir)
endif()
