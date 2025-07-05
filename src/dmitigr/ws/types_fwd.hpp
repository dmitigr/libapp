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

#ifndef DMITIGR_WS_TYPES_FWD_HPP
#define DMITIGR_WS_TYPES_FWD_HPP

/// The API.
namespace dmitigr::ws {

enum class Data_format;

class Connection;
class Exception;
class Http_io;
class Http_request;
class Server;
class Server_options;

/// The implementation details.
namespace detail {
struct Ws_data;

class iConnection;
template<bool> class Conn;

class iHttp_request;

class iHttp_io;
template<bool> class iHttp_io_templ;

class iServer;
template<bool> class Srv;

class iServer_options;
} // namespace detail

} // namespace dmitigr::ws

#endif  // DMITIGR_WS_TYPES_FWD_HPP
