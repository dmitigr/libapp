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

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is generated automatically. Edit dll.hpp.in instead!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef DMITIGR_WS_DLL_HPP
#define DMITIGR_WS_DLL_HPP

#ifdef _WIN32
  #ifdef DMITIGR_WS_DLL_BUILDING
    #define DMITIGR_WS_API __declspec(dllexport)
  #else
    #if DMITIGR_WS_DLL
      #define DMITIGR_WS_API __declspec(dllimport)
    #else /* static or header-only library on Windows */
      #define DMITIGR_WS_API
    #endif
  #endif
#else /* Unix */
  #define DMITIGR_WS_API
#endif

#ifndef DMITIGR_WS_INLINE
  #if !defined(DMITIGR_WS_NOT_HEADER_ONLY) && !defined(DMITIGR_WS_BUILDING)
    #define DMITIGR_WS_INLINE inline
  #else
    #define DMITIGR_WS_INLINE
  #endif
#endif  // DMITIGR_WS_INLINE

#endif // DMITIGR_WS_DLL_HPP
