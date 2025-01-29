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

enable_language(C)
set(C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(UWSC_VERSION_MAJOR 3)
set(UWSC_VERSION_MINOR 3)
set(UWSC_VERSION_PATCH 5)

set(uwsc_headers
  buffer.h
  config.h
  sha1.h
  utils.h
  uwsc.h
  )
set(uwsc_implementations
  buffer.c
  sha1.c
  utils.c
  uwsc.c
  )

if(DMITIGR_LIBS_AIO STREQUAL "uv")
  find_package(Uv REQUIRED)
  set(uwsc_link_libraries_public ${Uv_LIBRARIES} ${Uv_EXTRA_LIBRARIES})
  set(uwsc_include_directories_public ${Uv_INCLUDE_DIRS})

  set(UWSC_USE_UV On) #define UWSC_USE_UV in config.h by CMake.
else()
  find_package(Ev REQUIRED)
  set(uwsc_link_libraries_public ${Ev_LIBRARIES})
  set(uwsc_include_directories_public ${Ev_INCLUDE_DIRS})
endif()

if(DMITIGR_LIBS_OPENSSL)
  find_package(OpenSSL REQUIRED)
  list(APPEND uwsc_link_libraries_public OpenSSL::SSL OpenSSL::Crypto)
  list(APPEND uwsc_headers ssl.h)
  list(APPEND uwsc_implementations openssl.c)

  set(SSL_SUPPORT On) #define SSL_SUPPORT in config.h by CMake.
endif()
foreach(type headers implementations)
  list(TRANSFORM uwsc_${type} PREPEND "${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/uwsc/")
endforeach()

if(MSVC)
  list(APPEND uwsc_cflags /W0)
elseif(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND uwsc_cflags
    -fvisibility=hidden
    -Wall
    -Wextra
    #-Wstrict-prototypes
    -Wno-implicit-fallthrough
    -Wno-sign-compare
    -Wno-unused-parameter)
endif()

if(WIN32)
  list(APPEND uwsc_defines WIN32_LEAN_AND_MEAN)
else()
  list(APPEND uwsc_defines _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE _GNU_SOURCE)
endif()

configure_file(${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/uwsc/config.h.in
  ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/uwsc/config.h @ONLY
  NEWLINE_STYLE UNIX)

# ------------------------------------------------------------------------------

set(dmlib dmitigr_3rdparty_uwsc)

if(DMITIGR_LIBS_HEADER_ONLY)
  # In header-only mode we build a static library to be exported.
  set(dmlib_type STATIC)
  set(dmlib_export_type "static")
else()
  # In not header-only mode build an object library which will be exported as
  # interface automatically. The object library will be included (directly
  # linked) into the our libraries. The interface definition of the object
  # library will contains compile definitions.
  set(dmlib_type OBJECT)
  set(dmlib_export_type "interface")
endif()

add_library(${dmlib} ${dmlib_type} ${uwsc_implementations} ${uwsc_headers})
set_target_properties(${dmlib}
  PROPERTIES
  OUTPUT_NAME "${dmlib}"
  LINKER_LANGUAGE "C"
  POSITION_INDEPENDENT_CODE True
  # DEBUG_POSTFIX "d"
  )
target_compile_definitions(${dmlib}
  PUBLIC ${uwsc_compile_definitions_public}
  PRIVATE ${uwsc_defines})
target_compile_options(${dmlib} PRIVATE ${uwsc_cflags})
target_include_directories(${dmlib} PUBLIC ${uwsc_include_directories_public})
target_link_libraries(${dmlib} PUBLIC ${uwsc_link_libraries_public})

if(DMITIGR_LIBS_INSTALL)
  install(FILES ${uwsc_headers}
    DESTINATION "${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/3rdparty/uwsc")
  install(TARGETS ${dmlib}
    EXPORT ${dmlib}_export
    ARCHIVE DESTINATION ${DMITIGR_LIBS_LIB_INSTALL_DIR})
  install(EXPORT ${dmlib}_export
    DESTINATION "${DMITIGR_LIBS_CMAKE_INSTALL_DIR}"
    FILE "${dmlib}_${dmlib_export_type}-config.cmake")
endif()
