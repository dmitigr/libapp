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

#ifndef DMITIGR_NIX_NIX_HPP
#define DMITIGR_NIX_NIX_HPP

#if defined(__linux__) || defined(__APPLE__)
#include "detach.hpp"
#include "ifaddrs.hpp"
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include "sysctl.hpp"
#endif

#include "error.hpp"
#include "ipc_pipe.hpp"
#include "process.hpp"

#endif  // DMITIGR_NIX_NIX_HPP
