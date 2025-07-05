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

#ifndef DMITIGR_MSG_PROCESSOR_HPP
#define DMITIGR_MSG_PROCESSOR_HPP

#include "types_fwd.hpp"

#include <memory>

namespace dmitigr::msg {

class Processor {
public:
  /// The destructor.
  virtual ~Processor() = default;

  /// @returns The copy of the instance.
  virtual std::unique_ptr<Processor> clone() = 0;

  /// @returns The exit code. (0 indicates success.)
  virtual int process(const Message& message) = 0;
};

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_PROCESSOR_HPP
