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

#ifndef DMITIGR_UV_UV_HPP
#define DMITIGR_UV_UV_HPP

#include "../base/assert.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <uv.h>

#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

namespace dmitigr::uv {

/// Wrapper around uv_handle_get_data().
template<typename T, typename H>
T* data(H* handle) noexcept
{
  return static_cast<T*>(uv_handle_get_data(
    reinterpret_cast<uv_handle_t*>(handle)));
}

/// Wrapper around uv_handle_set_data().
template<typename H>
void set_data(H* handle, void* data) noexcept
{
  uv_handle_set_data(reinterpret_cast<uv_handle_t*>(handle), data);
}

/// RAII wrapper around uv_loop_t.
class Loop final {
public:
  /// Closes the loop.
  ~Loop()
  {
    close();
  }

  /// Initializes the loop.
  Loop() noexcept
    : init_error_{uv_loop_init(native())}
  {}

  /// Non copy-constructible.
  Loop(const Loop&) = delete;
  /// Non copy-assignable.
  Loop& operator=(const Loop&) = delete;
  /// Non move-constructible.
  Loop(Loop&&) = delete;
  /// Non move-assignable.
  Loop& operator=(Loop&&) = delete;

  /// @returns Native handle.
  const uv_loop_t* native() const noexcept
  {
    return &native_;
  }

  /// @overload
  uv_loop_t* native() noexcept
  {
    return &native_;
  }

  /// @returns Initialization error.
  int init_error() const noexcept
  {
    return init_error_;
  }

  /// Runs the event loop.
  int run(const uv_run_mode mode) noexcept
  {
    return uv_run(native(), mode);
  }

  /// Stops the event loop.
  void stop() noexcept
  {
    uv_stop(native());
  }

  /// Releases all internal loop resources.
  int close() noexcept
  {
    return uv_loop_close(native());
  }

private:
  int init_error_{};
  uv_loop_t native_;
};

/// RAII wrapper around uv_handle_t.
template<typename T>
class Handle {
public:
  /// Closes the handle.
  ~Handle()
  {
    if (native_.type)
      uv_close(reinterpret_cast<uv_handle_t*>(native()), close_cb_);
  }

  /// The constructor.
  explicit Handle(uv_close_cb cb = {}) noexcept
    : close_cb_{cb}
  {
    std::memset(&native_, 0, sizeof(native_));
  }

  /// Non copy-constructible.
  Handle(const Handle&) = delete;
  /// Non copy-assignable.
  Handle& operator=(const Handle&) = delete;
  /// Non move-constructible.
  Handle(Handle&&) = delete;
  /// Non move-assignable.
  Handle& operator=(Handle&&) = delete;

  /// @returns Native handle.
  const T* native() const noexcept
  {
    return &native_;
  }

  /// @overload
  T* native() noexcept
  {
    return &native_;
  }

  /// @returns Initialization error.
  int init_error() const noexcept
  {
    return init_error_;
  }

protected:
  /// Initializes the handle.
  template<typename Init>
  Handle(Loop& loop, const Init& init) noexcept
    : loop_{&loop}
    , init_error_{init(loop.native(), native())}
  {}

  /// Starts the handle.
  template<typename Start, typename Callback, typename ... Types>
  int start_handle(std::function<void()> callback,
    const Start& start, const Callback& cb, Types&& ... args) noexcept
  {
    if (!callback)
      return UV_EINVAL;
    callback_ = std::move(callback);
    set_data(native(), this);
    const auto r = start(native(), &cb, std::forward<Types>(args)...);
    if (r) {
      set_data(native(), nullptr);
      callback_ = {};
    }
    return r;
  }

  /// Stops the handle.
  template<typename Stop>
  int stop_handle(const Stop& stop) noexcept
  {
    const auto r = stop(native());
    set_data(native(), nullptr);
    callback_ = {};
    return r;
  }

  /// Invokes the stored callback.
  static void invoke_callback(T* h) noexcept
  {
    auto* const self = data<Handle>(h);
    DMITIGR_ASSERT(self && self->callback_);
    self->callback_();
  }

  /// @returns The loop the handle is running on.
  const Loop& loop() const noexcept
  {
    return *loop_;
  }

  /// @overload
  Loop& loop() noexcept
  {
    return *loop_;
  }

private:
  Loop* loop_{};
  int init_error_{};
  T native_;
  uv_close_cb close_cb_{};
  std::function<void()> callback_;
};

/// RAII wrapper around uv_timer_t.
class Timer final : public Handle<uv_timer_t> {
public:
  /// The alias of super class.
  using Super = Handle<uv_timer_t>;

  /// The constructor.
  Timer(Loop& loop) noexcept
    : Super{loop, uv_timer_init}
  {}

  /// @returns The loop the handle is running on.
  using Super::loop;

  /// Starts the handle.
  template<typename ... Types>
  int start(std::function<void()> callback, Types&& ... args) noexcept
  {
    return start_handle(std::move(callback), uv_timer_start, timer_cb,
      std::forward<Types>(args)...);
  }

  /// Stops the handle.
  int stop() noexcept
  {
    return stop_handle(uv_timer_stop);
  }

private:
  static void timer_cb(uv_timer_t* h)
  {
    invoke_callback(h);
  }
};

/// RAII wrapper around uv_signal_t.
class Signal final : public Handle<uv_signal_t> {
public:
  /// The alias of super class.
  using Super = Handle<uv_signal_t>;

  /// The constructor.
  Signal(Loop& loop) noexcept
    : Super{loop, uv_signal_init}
  {}

  /// @returns The loop the handle is running on.
  using Super::loop;

  /// Starts the handle.
  template<typename ... Types>
  int start(std::function<void()> callback, Types&& ... args) noexcept
  {
    return start_handle(std::move(callback), uv_signal_start, signal_cb,
      std::forward<Types>(args)...);
  }

  /// Stops the handle.
  int stop() noexcept
  {
    return stop_handle(uv_signal_stop);
  }

private:
  static void signal_cb(uv_signal_t* h, int /*signum*/)
  {
    invoke_callback(h);
  }
};

} // namespace dmitigr::uv

#endif  // DMITIGR_UV_UV_HPP
