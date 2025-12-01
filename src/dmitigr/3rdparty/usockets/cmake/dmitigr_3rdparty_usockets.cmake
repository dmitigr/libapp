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

enable_language(C)
set(C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if (DMITIGR_LIBS_OPENSSL)
  find_package(OpenSSL REQUIRED)
endif()

find_package(Uv REQUIRED)

if(MSVC)
  list(APPEND usockets_cflags /W0)
elseif(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND usockets_cflags
    -fvisibility=hidden
    -Wall
    -Wextra
    #-Wstrict-prototypes
    -Wno-sign-compare
    -Wno-unused-parameter)
endif()

set(usockets_headers
  internal/internal.h
  internal/loop_data.h
  internal/eventing/libuv.h
  internal/networking/bsd.h
  )
set(usockets_implementations
  bsd.c
  context.c
  loop.c
  socket.c
  socket_dmitigr.c
  udp.c
  eventing/libuv.c
  )
if (DMITIGR_LIBS_OPENSSL)
  list(APPEND usockets_implementations crypto/openssl.c)
  set(usockets_sni_tree dmitigr_3rdparty_usockets_sni_tree)
  add_library(${usockets_sni_tree} OBJECT
    ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/usockets/crypto/sni_tree.cpp)
  set_target_properties(${usockets_sni_tree}
    PROPERTIES
    LINKER_LANGUAGE "C"
    POSITION_INDEPENDENT_CODE True
    )
endif()
foreach(type headers implementations)
  list(TRANSFORM usockets_${type} PREPEND "${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/usockets/")
endforeach()

if(WIN32)
  list(APPEND usockets_defines WIN32_LEAN_AND_MEAN)
else()
  list(APPEND usockets_defines _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE)
endif()

# ------------------------------------------------------------------------------

set(dmlib dmitigr_3rdparty_usockets)

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

add_library(${dmlib} ${dmlib_type} ${usockets_implementations})
set_target_properties(${dmlib}
  PROPERTIES
  OUTPUT_NAME "${dmlib}"
  LINKER_LANGUAGE "C"
  POSITION_INDEPENDENT_CODE True
  # DEBUG_POSTFIX "d"
  )
target_compile_definitions(${dmlib}
  PRIVATE ${usockets_defines}
  PUBLIC LIBUS_USE_LIBUV)
target_compile_options(${dmlib} PRIVATE ${usockets_cflags})
target_include_directories(${dmlib}
  PUBLIC ${Uv_INCLUDE_DIRS}
  PRIVATE $<BUILD_INTERFACE:${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/usockets>)
target_link_libraries(${dmlib}
  PUBLIC ${Uv_LIBRARIES} ${Uv_EXTRA_LIBRARIES})
if(DMITIGR_LIBS_OPENSSL)
  target_compile_definitions(${dmlib} PRIVATE LIBUS_USE_OPENSSL)
  target_link_libraries(${dmlib}
    PUBLIC OpenSSL::SSL OpenSSL::Crypto
    PRIVATE $<TARGET_OBJECTS:${usockets_sni_tree}>)
else()
  target_compile_definitions(${dmlib} PRIVATE LIBUS_NO_SSL)
endif()

if(DMITIGR_LIBS_INSTALL)
  install(FILES
    ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/usockets/libusockets.h
    ${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/usockets/libusockets_dmitigr.h
    DESTINATION "${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/3rdparty/usockets")
  install(TARGETS ${dmlib} ${usockets_sni_tree}
    EXPORT ${dmlib}_export
    ARCHIVE DESTINATION ${DMITIGR_LIBS_LIB_INSTALL_DIR})
  install(EXPORT ${dmlib}_export
    DESTINATION "${DMITIGR_LIBS_CMAKE_INSTALL_DIR}"
    FILE "${dmlib}_${dmlib_export_type}-config.cmake")
endif()
