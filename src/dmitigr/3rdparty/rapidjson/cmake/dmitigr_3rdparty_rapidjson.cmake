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

set(dmlib dmitigr_3rdparty_rapidjson)

add_library(${dmlib} INTERFACE)

target_compile_definitions(${dmlib}
  INTERFACE
  RAPIDJSON_HAS_STDSTRING=1
  RAPIDJSON_NO_SIZETYPEDEFINE)

if(DMITIGR_LIBS_INSTALL)
  install(DIRECTORY "${dmitigr_libs_SOURCE_DIR}/${dmitigr_libs_subroot}/3rdparty/rapidjson/"
    DESTINATION "${DMITIGR_LIBS_INCLUDE_INSTALL_DIR}/dmitigr/3rdparty/rapidjson"
    FILES_MATCHING PATTERN "*.h")
  install(TARGETS ${dmlib}
    EXPORT ${dmlib}_export)
  install(EXPORT ${dmlib}_export
    DESTINATION ${DMITIGR_LIBS_CMAKE_INSTALL_DIR}
    FILE ${dmlib}_interface-config.cmake)
endif()
