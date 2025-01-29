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

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is generated automatically. Edit dll.hpp.in instead!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef DMITIGR_WSCL_DLL_HPP
#define DMITIGR_WSCL_DLL_HPP

#ifdef _WIN32
  #ifdef DMITIGR_WSCL_DLL_BUILDING
    #define DMITIGR_WSCL_API __declspec(dllexport)
  #else
    #if DMITIGR_WSCL_DLL
      #define DMITIGR_WSCL_API __declspec(dllimport)
    #else /* static or header-only library on Windows */
      #define DMITIGR_WSCL_API
    #endif
  #endif
#else /* Unix */
  #define DMITIGR_WSCL_API
#endif

#ifndef DMITIGR_WSCL_INLINE
  #if !defined(DMITIGR_WSCL_NOT_HEADER_ONLY) && !defined(DMITIGR_WSCL_BUILDING)
    #define DMITIGR_WSCL_INLINE inline
  #else
    #define DMITIGR_WSCL_INLINE
  #endif
#endif  // DMITIGR_WSCL_INLINE

#endif // DMITIGR_WSCL_DLL_HPP
