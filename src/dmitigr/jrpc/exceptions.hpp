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

#ifndef DMITIGR_JRPC_EXCEPTIONS_HPP
#define DMITIGR_JRPC_EXCEPTIONS_HPP

#include "../base/exceptions.hpp"

namespace dmitigr::jrpc {

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public dmitigr::Exception {
  using dmitigr::Exception::Exception;
};

} // namespace dmitigr::jrpc

#endif  // DMITIGR_JRPC_EXCEPTIONS_HPP
