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
#pragma comment(lib, "userenv")

#include "error.hpp"

#include <memory>

#include <userenv.h>

namespace dmitigr::winbase {

class Environment_block final {
public:
  Environment_block() = default;

  explicit Environment_block(const bool inherit)
    : Environment_block{NULL, inherit}
  {}

  Environment_block(const HANDLE token, const bool inherit)
  {
    void* env{};
    if (!CreateEnvironmentBlock(&env, token, inherit))
      throw std::runtime_error{last_error_message()};
    env_.reset(env);
  }

  void* data() const noexcept
  {
    return env_.get();
  }

private:
  struct Deleter final {
    void operator()(void* const env) const noexcept
    {
      DestroyEnvironmentBlock(env);
    }
  };
  std::unique_ptr<void, Deleter> env_;
};

} // namespace dmitigr::winbase
