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

#ifndef DMITIGR_WS_UWEBSOCKETS_HPP
#define DMITIGR_WS_UWEBSOCKETS_HPP

#ifdef __GNUG__
// Disable some warnings of uWebSockets.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "../3rdparty/uwebsockets/App.h"
#include "../3rdparty/uwebsockets/HttpParser.h"
#include "../3rdparty/uwebsockets/HttpResponse.h"

#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif

#endif  // DMITIGR_WS_UWEBSOCKETS_HPP
