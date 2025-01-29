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

#ifndef DMITIGR_MSG_CONVEYOR_HPP
#define DMITIGR_MSG_CONVEYOR_HPP

#include "types_fwd.hpp"

namespace dmitigr::msg {

/**
 * @par Thread safety
 * All the functions are thread-safe.
 */
class Conveyor {
public:
  /// The destructor.
  virtual ~Conveyor() = default;

  /// Starts conveyor.
  virtual void start() = 0;

  /// Stops conveyor.
  virtual void stop() = 0;

  /// @returns `true` if this conveyor instance is started.
  virtual bool is_started() const noexcept = 0;

  /**
   * @brief Puts to message to the spool for further processing.
   *
   * @par Requires
   * `is_started()`.
   */
  virtual void spool(const Message& message) = 0;

  /**
   * @brief Passes the next message to a processor.
   *
   * @returns `true` if another one message was processed, or `false` if there
   * was no message to process.
   *
   * @par Requires
   * `is_started()`.
   */
  virtual bool process(Processor& processor) = 0;
};

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_CONVEYOR_HPP
