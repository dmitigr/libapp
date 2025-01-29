// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
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

#ifndef DMITIGR_RAJSON_FWD_HPP
#define DMITIGR_RAJSON_FWD_HPP

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

#include "../3rdparty/rapidjson/fwd.h"

#endif  // DMITIGR_RAJSON_FWD_HPP
