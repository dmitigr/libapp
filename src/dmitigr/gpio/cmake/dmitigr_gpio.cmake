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
# Pre-checking
# ------------------------------------------------------------------------------

if(NOT LINUX)
  set(dmitigr_gpio_not_available True)
  return()
endif()

# ------------------------------------------------------------------------------
# Info
# ------------------------------------------------------------------------------

dmitigr_libs_set_library_info(gpio 1 0 0 "GPIO library")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_gpio_headers
  gpio.hpp
  exceptions.hpp
)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_gpio_deps base)

if(DMITIGR_LIBS_GPIOD)
  find_package(Gpiod REQUIRED)
  list(APPEND dmitigr_gpio_target_include_directories_public "${Gpiod_INCLUDE_DIRS}")
  list(APPEND dmitigr_gpio_target_include_directories_interface "${Gpiod_INCLUDE_DIRS}")
  list(APPEND dmitigr_gpio_target_link_libraries_public ${Gpiod_LIBRARIES})
  list(APPEND dmitigr_gpio_target_link_libraries_interface ${Gpiod_LIBRARIES})
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_gpio_tests base)
  set(dmitigr_gpio_tests_target_link_libraries dmitigr_base)
endif()
