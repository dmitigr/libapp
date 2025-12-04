// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DMITIGR_RAJSON_RAPIDJSON_HPP
#define DMITIGR_RAJSON_RAPIDJSON_HPP

#include <cstddef>
#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif
#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
#endif

namespace rapidjson {
using SizeType = std::size_t;
} // namespace rapidjson

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include "../3rdparty/rapidjson/fwd.h"
#include "../3rdparty/rapidjson/document.h"
#include "../3rdparty/rapidjson/error/en.h"
#include "../3rdparty/rapidjson/error/error.h"
#include "../3rdparty/rapidjson/schema.h"
#include "../3rdparty/rapidjson/stringbuffer.h"
#include "../3rdparty/rapidjson/writer.h"
#ifdef __clang__
#pragma GCC diagnostic pop
#endif

#endif  // DMITIGR_RAJSON_RAPIDJSON_HPP
