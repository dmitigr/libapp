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

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
/*
 * For historical reasons, the Windows.h header defaults to including the
 * Winsock.h header file for Windows Sockets 1.1. The declarations in the
 * Winsock.h header file will conflict with the declarations in the Winsock2.h
 * header file required by Windows Sockets 2.0. The WIN32_LEAN_AND_MEAN macro
 * prevents the Winsock.h from being included by the Windows.h header.
 *
 * https://docs.microsoft.com/en-us/windows/desktop/winsock/include-files-2
 * https://social.msdn.microsoft.com/Forums/vstudio/en-US/671124df-c42b-48b8-a4ac-3413230bc43b/dll-compilationredefinition-error
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <string>

namespace dmitigr::winbase {

DWORD last_error() noexcept;
std::wstring system_message_w(DWORD);
std::string system_message(DWORD);
std::wstring last_error_message_w();
std::string last_error_message();

} // namespace dmitigr::winbase
