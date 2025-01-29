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

#ifndef DMITIGR_BASE_STACK_HPP
#define DMITIGR_BASE_STACK_HPP

#include <deque>
#include <utility>

namespace dmitigr {

/// A scope stack.
template<typename T, class Container = std::deque<T>>
struct Scope_stack final {
  /// An Scope_stack guard.
  struct Guard final {
    /// Removes the top element.
    ~Guard()
    {
      self_.stack_.pop_back();
    }

    /// Not copy-constructible.
    Guard(const Guard&) = delete;
    /// Not copy-assignable.
    Guard& operator=(const Guard&) = delete;
    /// Not move-constructible.
    Guard(Guard&&) = delete;
    /// Not move-assignable.
    Guard& operator=(Guard&&) = delete;

  private:
    friend Scope_stack;

    Scope_stack& self_;

    template<typename U>
    Guard(Scope_stack& self, U&& element)
      : self_{self}
    {
      self_.stack_.emplace_back(std::forward<U>(element));
    }
  };

  /**
   * @brief Inserts `element` at the top.
   *
   * @returns An instance of type Guard which is responsible to remove the
   * inserted `element` when control leaves the scope in which the Guard
   * instance was created.
   */
  template<typename U>
  Guard push(U&& element)
  {
    return Guard{*this, std::forward<U>(element)};
  }

  /// The constructor.
  Scope_stack(Container stack)
    : stack_{std::move(stack)}
  {}

  /// @returns The reference to the underlying container.
  const Container& container() const noexcept
  {
    return stack_;
  }

  /**
   * @returns The underlying container.
   *
   * @par Effects
   * `container().empty()`.
   */
  Container release() noexcept
  {
    Container result;
    stack_.swap(result);
    return std::move(result);
  }

private:
  Container stack_;
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_STACK_HPP
