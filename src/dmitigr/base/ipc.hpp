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

#ifndef DMITIGR_BASE_IPC_HPP
#define DMITIGR_BASE_IPC_HPP

#include <cstdint>
#include <string>

namespace dmitigr::ipc {

/// A message.
class Message {
public:
  /// A serialized message.
  struct Serialized final {
    std::int16_t format{};
    std::string bytes;
  };

  /// The destructor.
  virtual ~Message() = default;

  /// @returns The message identifier.
  virtual std::int64_t id() const noexcept = 0;

  /// @returns A message serialization.
  virtual Serialized to_serialized() const = 0;
};

/// A request message.
class Request : public Message {};

/// A response message.
class Response : public Message {};

/// An error response message.
class Error : public Response {
public:
  /// Throws exception by using this instance.
  [[noreturn]] virtual void throw_from_this() const = 0;
};

} // namespace dmitigr::ipc

#endif  // DMITIGR_BASE_IPC_HPP
