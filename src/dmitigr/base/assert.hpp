// -*- C++ -*-
//
// Copyright 2026 Dmitry Igrishin
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

#ifndef DMITIGR_BASE_ASSERT_HPP
#define DMITIGR_BASE_ASSERT_HPP

#ifndef DMITIGR_ASSERT
#include <exception> // std::terminate
#include <iostream>
#endif

#ifndef DMITIGR_CKARG
#include <source_location>
#endif

#if !defined(DMITIGR_CKARG) || !defined(DMITIGR_CHECK)
#include <stdexcept>
#include <string>
#endif

#ifndef DMITIGR_ASSERT
/**
 * @brief Checks the assertion `a`.
 *
 * @details Calls std::terminate() if assertion `a` violated.
 *
 * @remarks Always active regardless of `NDEBUG`.
 *
 * @par Effects
 * If `!a`, an assertion is printed with the name of the source file and the
 * line where it was violated, after which std::terminate() is called.
 */
#define DMITIGR_ASSERT(a) do {                                          \
    if (!(a)) {                                                         \
      std::cerr<<"assertion ("#a") failed at "<<__FILE__<<":"<<__LINE__<<"\n"; \
      std::terminate();                                                 \
    }                                                                   \
  } while (false)
#endif  // DMITIGR_ASSERT

#ifndef DMITIGR_CHECK
/**
 * @brief Checks the assertion `a`.
 *
 * @details Throws std::logic_error if assertion `a` violated.
 *
 * @remarks Always active regardless of `NDEBUG`.
 *
 * @throws std::logic_error with assertion text, the name of the source file and
 * the line where it was violated as the what-string.
 */
#define DMITIGR_CHECK(a) do {                                           \
    if (!(a)) {                                                         \
      throw std::logic_error{std::string{"check ("#a") failed at "}     \
        .append(__FILE__).append(1, ':').append(std::to_string(__LINE__))}; \
    }                                                                   \
  } while (false)
#endif  // DMITIGR_CHECK

#ifndef DMITIGR_CKARG
/**
 * @brief Checks the assertion `a`.
 *
 * @details Throws std::invalid_argument if assertion `a` violated.
 *
 * @remarks Always active regardless of `NDEBUG`.
 *
 * @throws std::invalid_argument with assertion text and the function name (if
 * used in a function body) where it was violated as the what-string.
 */
#define DMITIGR_CKARG(a) do { if (!(a)) { const auto sl = std::source_location::current(); \
      std::string msg;                                                  \
      msg.reserve(192);                                                 \
      msg.append("argument check ("#a") failed");                       \
      if (sl.function_name())                                           \
        msg.append(" in ").append(sl.function_name());                  \
      throw std::invalid_argument{msg};                                 \
    }} while (false)
#endif  // DMITIGR_CKARG

#ifndef DMITIGR_THROW_INVARG
/**
 * @brief Checks the assertion `a`.
 *
 * @throws std::invalid_argument with text `a` and the function name (if
 * used in a function body).
 */
#define DMITIGR_THROW_INVARG(a) do { const auto sl = std::source_location::current(); \
    (void)(a);                                                          \
    std::string msg;                                                    \
    msg.reserve(192);                                                   \
    msg.append("invalid argument ("#a")");                              \
    if (sl.function_name())                                             \
      msg.append(" of ").append(sl.function_name());                    \
    throw std::invalid_argument{msg};                                   \
  } while (false)
#endif  // DMITIGR_THROW_INVARG

#endif  // DMITIGR_BASE_ASSERT_HPP
