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

cmake_minimum_required(VERSION 3.16)
cmake_policy(VERSION 3.16)
project(dmitigr_libs)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
  cmake_policy(SET CMP0144 NEW)
endif()

if (NOT (UNIX OR WIN32))
  message(FATAL_ERROR "dmitigr_libs: unsupported platform")
endif()

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/src")
include(dmitigr)
include(dmitigr_libs)

# ------------------------------------------------------------------------------
# Build options
# ------------------------------------------------------------------------------

set(DMITIGR_LIBS_ROOT "../src" CACHE PATH
  "The libraries source root directory relative to ${CMAKE_CURRENT_SOURCE_DIR}")
set(dmitigr_libs_subroot "${DMITIGR_LIBS_ROOT}/dmitigr")
set(DMITIGR_LIBS_CLANG_USE_LIBCPP On CACHE BOOL
  "Use libc++ with Clang?")
set(DMITIGR_LIBS_HEADER_ONLY Off CACHE BOOL
  "Whole header-only?")
set(DMITIGR_LIBS_DOXYGEN Off CACHE BOOL
  "Build configurations for Doxygen?")
set(DMITIGR_LIBS_TESTS Off CACHE BOOL
  "Build tests?")
set(DMITIGR_LIBS_GPIOD On CACHE BOOL
  "Link libgpiod where required?")
set(DMITIGR_LIBS_PQ On CACHE BOOL
  "Link libpq where required?")
set(DMITIGR_LIBS_SQLITE3 On CACHE BOOL
  "Link SQLite3 where required?")
set(DMITIGR_LIBS_OPENSSL Off CACHE BOOL
  "Link to OpenSSL where required?")
set(DMITIGR_LIBS_ZLIB Off CACHE BOOL
  "Link to Zlib where required?")
set(DMITIGR_LIBS_AIO "uv" CACHE STRING
  "What AIO to use? (\"uv\" - is the only option now.)")
set(BUILD_SHARED_LIBS Off CACHE BOOL
  "Build shared libraries?")
set(CMAKE_VERBOSE_MAKEFILE On CACHE BOOL
  "Verbose output upon build?")
set(DMITIGR_LIBS_INCLUDE "" CACHE STRING
  "A list of libraries to include (empty value means \"all available\")")
set(DMITIGR_LIBS_EXCLUDE "" CACHE STRING
  "A list of libraries to exclude")

foreach(suff INCLUDE EXCLUDE)
  list(REMOVE_DUPLICATES DMITIGR_LIBS_${suff})
endforeach()

# Enable linking with libc++ if requested.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (DMITIGR_LIBS_CLANG_USE_LIBCPP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

# Set default CMAKE_BUILD_TYPE.
if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Build type: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

if (NOT DMITIGR_LIBS_HEADER_ONLY)
  if(BUILD_SHARED_LIBS)
    message("dmitigr_libs: shared libraries requested (${CMAKE_BUILD_TYPE})")
  else()
    message("dmitigr_libs: static libraries requested (${CMAKE_BUILD_TYPE})")
  endif()
elseif(BUILD_SHARED_LIBS)
  message(FATAL_ERROR "dmitigr_libs: DMITIGR_LIBS_HEADER_ONLY and BUILD_SHARED_LIBS are mutually exclusive")
else()
  message("dmitigr_libs: only 3rdparties without header-only option will be built")
endif()

if(DMITIGR_LIBS_DOXYGEN)
  message("dmitigr_libs: Doxygen configurations requested")
endif()

if(DMITIGR_LIBS_TESTS)
  message("dmitigr_libs: tests requested (${CMAKE_BUILD_TYPE})")
endif()

string(REGEX MATCH "(uv)$" aio "${DMITIGR_LIBS_AIO}")
if(NOT aio)
  message(FATAL_ERROR "dmitigr_libs: invalid AIO -- \"${DMITIGR_LIBS_AIO}\"")
endif()

# ------------------------------------------------------------------------------

set(DMITIGR_LIBS_INSTALL On CACHE BOOL
  "Install the stuff?")

if(UNIX)
  set(DMITIGR_LIBS_SHARE_INSTALL_DIR "share/dmitigr_libs" CACHE
    STRING "Name of the installation directory for the shared stuff relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_CMAKE_INSTALL_DIR "${DMITIGR_LIBS_SHARE_INSTALL_DIR}/cmake" CACHE
    STRING "Name of the installation directory for the CMake stuff relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_LIB_INSTALL_DIR "lib" CACHE
    STRING "Name of the installation directory for the libraries relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_INCLUDE_INSTALL_DIR "include" CACHE
    STRING "Name of the installation directory for the includes relative to ${CMAKE_INSTALL_PREFIX}")
elseif(WIN32)
  # On Windows, CMAKE_INSTALL_PREFIX is $ENV{ProgramFiles}\\${CMAKE_PROJECT_NAME} by default. In turn:
  #   - on AMD64: ProgramFiles=%ProgramFiles%
  #   - on   x86: ProgramFiles=%ProgramFiles(x86)%
  # See: https://msdn.microsoft.com/en-us/library/aa384274.aspx
  set(DMITIGR_LIBS_SHARE_INSTALL_DIR "." CACHE
    STRING "Name of the installation directory for the shared stuff relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_CMAKE_INSTALL_DIR "cmake" CACHE
    STRING "Name of the installation directory for the CMake stuff relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_LIB_INSTALL_DIR "lib" CACHE
    STRING "Name of the installation directory for the libraries relative to ${CMAKE_INSTALL_PREFIX}")
  set(DMITIGR_LIBS_INCLUDE_INSTALL_DIR "include" CACHE
    STRING "Name of the installation directory for the includes relative to ${CMAKE_INSTALL_PREFIX}")
endif()

# ------------------------------------------------------------------------------
# Languages
# ------------------------------------------------------------------------------

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# Get the actual list of libraries
# ------------------------------------------------------------------------------

file(GLOB dmitigr_libs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}"
  LIST_DIRECTORIES True "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/[a-z]*")
foreach(lib ${dmitigr_libs})
  string(REGEX MATCH "^[a-z][a-z0-9]+$" valid_lib_name "${lib}")
  if(NOT DMITIGR_LIBS_INCLUDE STREQUAL "" AND NOT "${lib}" IN_LIST DMITIGR_LIBS_INCLUDE)
    list(REMOVE_ITEM dmitigr_libs "${lib}")
  endif()
  if(NOT valid_lib_name OR "${lib}" IN_LIST DMITIGR_LIBS_EXCLUDE)
    list(REMOVE_ITEM dmitigr_libs "${lib}")
  endif()
endforeach()
unset(valid_lib_name)
message("dmitigr_libs: ${dmitigr_libs}")
if(DMITIGR_LIBS_EXCLUDE)
  message("dmitigr_libs: exlicitly excluded: ${DMITIGR_LIBS_EXCLUDE}")
endif()

# ------------------------------------------------------------------------------
# Output settings
# ------------------------------------------------------------------------------

# Note: Multi-configuration generators (VS, Xcode) appends a per-configuration
# subdirectory to CMAKE_RUNTIME_OUTPUT_DIRECTORY unless a generator expression
# is used.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if (WIN32)
  set(dmitigr_libs_resource_destination_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
elseif (UNIX)
  set(dmitigr_libs_resource_destination_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# ------------------------------------------------------------------------------
# Custom targets
# ------------------------------------------------------------------------------

add_custom_target(dmitigr_libs_uninstall)

add_custom_target(dmitigr_libs_create_resource_destination_dir ALL
  COMMAND cmake -E make_directory "${dmitigr_libs_resource_destination_dir}"
  )

# --------------------------------------
# Installing
# --------------------------------------

if(DMITIGR_LIBS_INSTALL)
  install(FILES LICENSE.txt
    DESTINATION "${DMITIGR_LIBS_SHARE_INSTALL_DIR}")
  install(FILES
    dmitigr_libs.cmake
    dmitigr_libs-config.cmake
    DESTINATION "${DMITIGR_LIBS_CMAKE_INSTALL_DIR}")
endif()

# ------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------

# @param file_list A file list name, e.g. "dmitigr_base_headers".
function(dmitigr_install_lib_includes file_list)
  foreach(file ${${file_list}})
    string(REGEX REPLACE "^${dmitigr_libs_subroot}/" "" file "${file}")
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES "${dmitigr_libs_subroot}/${file}"
      DESTINATION "${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/${dir}")
  endforeach()
endfunction()

# @param lib A library name.
macro(dmitigr_process_lib lib)
  string(TOUPPER "${lib}" LIB)

  set(dmlib dmitigr_${lib})
  set(DMLIB DMITIGR_${LIB})

  string(FIND "${lib}" "3rdparty_" pos)
  if (pos EQUAL 0)
    set(is_3rdparty_lib True)
    string(REGEX REPLACE "^3rdparty_(.*)" "3rdparty/\\1" lib_path "${dep}")
  else()
    set(is_3rdparty_lib False)
    set(lib_path "${lib}")
  endif()

  include(
    "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib_path}/cmake/${dmlib}.cmake")

  IF(NOT ${dmlib}_not_available AND NOT is_3rdparty_lib)

    dmitigr_libs_set_library_info_lib_variables(${lib})

    # -------------
    # Documentation
    # -------------

    if(DMITIGR_LIBS_DOXYGEN)
      configure_file(Doxyfile.in
        "${CMAKE_CURRENT_SOURCE_DIR}/doxygen/${lib}/Doxyfile" @ONLY
        NEWLINE_STYLE UNIX)

      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/doxyfile.specific")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/doxyfile.specific"
          DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/doxygen/${lib}")
      else()
        file(TOUCH "${CMAKE_CURRENT_SOURCE_DIR}/doxygen/${lib}/doxyfile.specific")
      endif()
    endif()

    # -------------------------------------
    # Conditional preprocessing and sources
    # -------------------------------------

    # Length of 0 means that the library is header-only anyway.
    list(LENGTH ${dmlib}_implementations ${dmlib}_implementations_length)
    if(NOT ${${dmlib}_implementations_length} EQUAL 0)
      # Some preprocessed headers (like dll.hpp) must be included for every
      # library with implementation files.
      foreach(file dll.hpp)
        configure_file("src/${file}.in"
          "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/${file}" @ONLY
          NEWLINE_STYLE UNIX)
        list(APPEND ${dmlib}_headers ${file})
      endforeach()

      set(${dmlib}_header_only ${DMITIGR_LIBS_HEADER_ONLY})
    else()
      set(${dmlib}_header_only TRUE)
    endif()

    if(NOT ${dmlib}_header_only)
      if(BUILD_SHARED_LIBS)
        set(export_file_suffix "shared")
      else()
        set(export_file_suffix "static")
      endif()
    else() # header-only
      set(export_file_suffix "interface")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/${lib}.hpp")
      # Headers
      list(APPEND ${dmlib}_headers ${lib}.hpp)
    endif()

    if(NOT ${${dmlib}_header_only})
      # Transunits
      configure_file("src/lib.cpp.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/${lib}.cpp" @ONLY
        NEWLINE_STYLE UNIX)
      list(APPEND ${dmlib}_transunits "${lib}.cpp")
    endif()

    # Both major and minor versions of 0 means that the library handles
    # versioning by itself, so no version.* files needs to be configured.
    if(DEFINED ${dmlib}_version_major)
      if((${${dmlib}_version_major} GREATER 0) OR (${${dmlib}_version_minor} GREATER 0))
        # Preprocessing
        configure_file("src/version.hpp.in"
          "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/version.hpp" @ONLY
          NEWLINE_STYLE UNIX)
        list(APPEND ${dmlib}_headers version.hpp)

        if(NOT ${${dmlib}_header_only})
          configure_file("src/lib_version.cpp.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/lib_version.cpp" @ONLY
            NEWLINE_STYLE UNIX)
          list(APPEND ${dmlib}_implementations lib_version.cpp)
          configure_file("src/lib_version.hpp.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/lib_version.hpp" @ONLY
            NEWLINE_STYLE UNIX)
          list(APPEND ${dmlib}_headers lib_version.hpp)

          if(WIN32)
            configure_file("src/version.rc.in"
              "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/version.rc" @ONLY
              NEWLINE_STYLE UNIX)
            list(APPEND ${dmlib}_build_only_sources version.rc)
          endif()
        endif()
      endif()
    else()
      message(FATAL_ERROR "dmitigr_libs: variable ${dmlib}_version_major is not defined")
    endif()

    foreach(st ${dmitigr_libs_source_types})
      list(TRANSFORM ${dmlib}_${st} PREPEND "${dmitigr_libs_subroot}/${lib}/")
      list(APPEND ${dmlib}_sources ${${dmlib}_${st}})
    endforeach()

    set_source_files_properties(
      ${${dmlib}_implementations}
      ${${dmlib}_cmake_sources}
      ${${dmlib}_cmake_unpreprocessed}

      PROPERTIES
      HEADER_FILE_ONLY ON)

    # ------------------------------------
    # Targets
    # ------------------------------------

    if(NOT ${${dmlib}_header_only})
      add_library(${dmlib} ${${dmlib}_sources})
    else()
      add_library(${dmlib} INTERFACE)
    endif()
    add_library(dmitigr::${lib} ALIAS ${dmlib})

    if(NOT ${${dmlib}_header_only})
      set(output_name "${dmlib}")
      target_compile_definitions(${dmlib} PUBLIC ${DMLIB}_NOT_HEADER_ONLY)
      if(BUILD_SHARED_LIBS)
        target_compile_definitions(${dmlib}
          PRIVATE ${DMLIB}_DLL_BUILDING
          PUBLIC  ${DMLIB}_DLL)
      elseif(WIN32)
        set(output_name "${dmlib}_static")
      endif()

      set_target_properties(${dmlib}
        PROPERTIES
        OUTPUT_NAME "${output_name}"
        LINKER_LANGUAGE "CXX"
        POSITION_INDEPENDENT_CODE True
        VERSION ${${dmlib}_version_major}.${${dmlib}_version_minor}
        # DEBUG_POSTFIX "d"
      )

      dmitigr_target_compile_options(${dmlib})
    endif()

    if(NOT ${${dmlib}_header_only})
      target_compile_definitions(${dmlib}
        PRIVATE ${${dmlib}_target_compile_definitions_private}
        PUBLIC  ${${dmlib}_target_compile_definitions_public})

      target_include_directories(${dmlib}
        PRIVATE ${${dmlib}_target_include_directories_private}
        PUBLIC  ${${dmlib}_target_include_directories_public}
        PUBLIC  $<BUILD_INTERFACE:${${dmlib}_target_include_directories_public}>
        PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${DMITIGR_LIBS_ROOT}>)
    else() # header-only
      target_compile_definitions(${dmlib}
        INTERFACE ${${dmlib}_target_compile_definitions_interface})

      target_include_directories(${dmlib}
        INTERFACE ${${dmlib}_target_include_directories_interface}
        INTERFACE $<BUILD_INTERFACE:${${dmlib}_target_include_directories_interface}>
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${DMITIGR_LIBS_ROOT}>)
    endif()

    # ------------------------------------
    # Dependency linkage
    # ------------------------------------

    if(NOT ${${dmlib}_header_only})
      foreach(dep ${dmitigr_libs_${lib}_deps})
        target_link_libraries(${dmlib} PUBLIC dmitigr_${dep})
      endforeach()

      # Link with manually specified dependencies in ${lib}/CMakeLists.txt (if any)
      target_link_libraries(${dmlib}
        PRIVATE ${${dmlib}_target_link_libraries_private}
        PUBLIC  ${${dmlib}_target_link_libraries_public})
    else() # header-only
      foreach(dep ${dmitigr_libs_${lib}_deps})
        target_link_libraries(${dmlib} INTERFACE dmitigr_${dep})
      endforeach()

      # Link with manually specified dependencies in ${lib}/CMakeLists.txt (if any)
      target_link_libraries(${dmlib}
        INTERFACE ${${dmlib}_target_link_libraries_interface})
    endif()

    # ------------------------------------
    # Installing
    # ------------------------------------

    if(DMITIGR_LIBS_INSTALL)
      dmitigr_install_lib_includes(${dmlib}_headers)
      if(${${dmlib}_header_only})
        dmitigr_install_lib_includes(${dmlib}_implementations)
      endif()

      if(NOT ${${dmlib}_header_only})
        install(TARGETS ${dmlib}
          EXPORT ${dmlib}_export
          ARCHIVE  DESTINATION ${DMITIGR_LIBS_LIB_INSTALL_DIR}
          LIBRARY  DESTINATION ${DMITIGR_LIBS_LIB_INSTALL_DIR}
          RUNTIME  DESTINATION ${DMITIGR_LIBS_LIB_INSTALL_DIR})
      else()
        install(TARGETS ${dmlib}
          EXPORT ${dmlib}_export
          INCLUDES DESTINATION ${DMITIGR_LIBS_INCLUDE_INSTALL_DIR})
      endif()

      # Don't use NAMESPACE, since ${dmlib} contains "dmitigr_" prefix already
      # and it seems there is no way to omit it in order to use NAMESPACE...
      install(EXPORT ${dmlib}_export
        # NAMESPACE dmitigr::
        DESTINATION ${DMITIGR_LIBS_CMAKE_INSTALL_DIR}
        FILE ${dmlib}_${export_file_suffix}-config.cmake)
    endif()

    # ------------------------------------
    # Uninstalling
    # ------------------------------------

    add_custom_command(TARGET dmitigr_libs_uninstall PRE_BUILD
      COMMAND cmake -E rm -f ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_CMAKE_INSTALL_DIR}/${dmlib}*
      COMMAND cmake -E rm -f ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_LIB_INSTALL_DIR}/${dmlib}*
      COMMAND cmake -E rm -f ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_LIB_INSTALL_DIR}/lib${dmlib}*
      COMMAND cmake -E rm -rf ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/${lib}*
      COMMAND cmake -E rm -rf ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/${lib})

  ENDIF()
endmacro()

macro(dmitigr_process_lib_and_deps lib)
  dmitigr_process_lib("${lib}")

  dmitigr_libs_get_deps(res ${lib})
  foreach(dep ${res})
    string(FIND "${dep}" "3rdparty_" pos)
    if(pos EQUAL 0)
      string(REGEX REPLACE "^3rdparty_(.*)" "3rdparty/\\1" dep_path "${dep}")

      string(SUBSTRING "${dep}" 9 -1 dep_wo_pref)
      add_custom_command(TARGET dmitigr_libs_uninstall PRE_BUILD
        COMMAND cmake -E rm -rf ${CMAKE_INSTALL_PREFIX}/${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/3rdparty/*${dep_wo_pref}*)
    else()
      set(dep_path "${dep}")
    endif()

    # Check that the dependency is present.
    set(pref "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${dep_path}")
    if((NOT EXISTS "${pref}") AND (NOT EXISTS "${pref}.hpp"))
      message(FATAL_ERROR "dmitigr_libs: ${lib} is depends on ${dep} which is not present")
    endif()

    if(NOT "${dep}" IN_LIST dmitigr_libs AND NOT "${dep}" IN_LIST dmitigr_libs_deps)
      dmitigr_process_lib_and_deps("${dep}")
      list(APPEND dmitigr_libs_deps "${dep}")
      set(dmitigr_libs_deps "${dmitigr_libs_deps}" PARENT_SCOPE)
    endif()
  endforeach()
endmacro()

foreach(lib ${dmitigr_libs})
  dmitigr_process_lib_and_deps(${lib})
endforeach()

message("dmitigr_libs: dependencies: ${dmitigr_libs_deps}")

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  enable_testing()

  function(dmitigr_configure_test lib test)
    set(dmlib dmitigr_${lib})
    set(full_name "${lib}-${test}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/test/${full_name}.cpp")
      set(is_unit_test TRUE)
      set(full_name "${lib}-unit-${test}")
    else()
      set(is_unit_test FALSE)
    endif()
    set(exe "dmitigr_${full_name}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${dmitigr_libs_subroot}/${lib}/test/${full_name}.cpp")
    add_executable(${exe} ${src})
    target_link_libraries(${exe} PRIVATE ${dmlib}
      ${${dmlib}_tests_target_link_libraries}
      ${${dmlib}_test_${test}_target_link_libraries})
    target_compile_definitions(${exe} PRIVATE
      ${${dmlib}_tests_target_compile_definitions}
      ${${dmlib}_test_${test}_target_compile_definitions})
    target_compile_options(${exe} PRIVATE
      ${${dmlib}_tests_target_compile_options}
      ${${dmlib}_test_${test}_target_compile_options})
    dmitigr_target_compile_options(${exe})
    if(is_unit_test)
      add_test(NAME ${exe} COMMAND ${exe})
    endif()
  endfunction()

  # Get the actual list of libraries with tests and configure these tests
  foreach(lib ${dmitigr_libs})
    set(dmlib dmitigr_${lib})
    if(${dmlib}_tests)
      list(APPEND libraries_with_tests ${lib})
      foreach(test ${${dmlib}_tests})
        dmitigr_configure_test(${lib} ${test})
      endforeach()
    endif()
  endforeach()

  # Set dependency lists of library tests
  macro(dmitigr_append_test_deps deps)
    foreach(dep ${${deps}})
      string(FIND "${dep}" "dmitigr_" pos)
      if(pos EQUAL 0)
        string(REGEX REPLACE "^dmitigr_(.*)" "\\1" dep "${dep}")
      endif()
      if(NOT "${dep}" IN_LIST ${lib}_test_deps)
        list(APPEND ${lib}_test_deps ${dep})
      endif()
    endforeach()
  endmacro()
  foreach(lib ${libraries_with_tests})
    set(dmlib dmitigr_${lib})
    dmitigr_append_test_deps(${dmlib}_tests_target_link_libraries)
    foreach(test ${${dmlib}_tests})
      dmitigr_append_test_deps(${dmlib}_test_${test}_target_link_libraries)
    endforeach()
  endforeach()
endif()
