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

#include "../base/ipc.hpp"
#include "windows.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace dmitigr::winbase::ipc::wm {

class Messenger final {
public:
  /**
   * @brief A message handler.
   *
   * @details A handler must determine the type of message. Since the responses
   * are handled by Messenger, if `data` represents a Response it must be just
   * created and returned. A `format` represents a `dwData` field of the
   * `COPYDATASTRUCT` structure.
   */
  using Handler = std::function<
    std::unique_ptr<ipc::Response>(HWND sender, std::string_view data, int format)>;

  ~Messenger()
  {
    stop();
  }

  void init(const std::wstring& clss, Handler handler, HINSTANCE instance = {})
  {
    const std::lock_guard lg{mutex_};

    if (!handler)
      throw std::invalid_argument{"cannot initialize ipc::wm::Messenger: "
        "invalid handler"};

    if (instance_)
      throw std::logic_error{"instance ipc::wm::Messenger already initialized"};
    else if (!instance)
      instance = GetModuleHandleW(nullptr);

    if (!register_window(instance, clss))
      throw std::runtime_error{"cannot register message class of ipc::wm::Messenger:" +
        last_error_message()};

    clss_ = clss;
    instance_ = instance;
    handler_ = [handle = std::move(handler)](const HWND sender,
      const std::string_view message, const int format)
        -> std::unique_ptr<ipc::Response>
      {
        try {
          return handle(sender, message, format);
        } catch (...) {}
        return nullptr;
      };
  }

  int run()
  {
    const auto main = [this]
    {
      const std::lock_guard lg{mutex_};

      if (!instance_)
        throw std::logic_error{"instance ipc::wm::Messenger not initialized"};

      if (window_)
        throw std::logic_error{"instance ipc::wm::Messenger already running"};

      assert(!clss_.empty());

      window_ = CreateWindowExW(
        0,
        clss_.c_str(), nullptr,
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        instance_,
        this);
      if (!window_)
        throw std::runtime_error{"cannot create message window of "
          "ipc::wm::Messenger: " + last_error_message()};

      if (!ChangeWindowMessageFilterEx(window_, WM_COPYDATA, MSGFLT_ALLOW, nullptr))
        throw std::runtime_error{"cannot modify UIPI message filter of "
          "ipc::wm::Messenger: " + last_error_message()};

      return window_;
    }();

    if (!SetTimer(main, cleanup_timer_id_, 1000, nullptr)) {
      const auto errmsg = last_error_message();
      stop();
      throw std::runtime_error{"cannot start cleanup timer of ipc::wm::Messenger: "
        + errmsg};
    }

    MSG msg;
    while (true) {
      msg = {};
      if (const int r{GetMessage(&msg, main, 0, 0)}; r == -1 || r == 0)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
  }

  void stop() noexcept
  {
    const std::lock_guard lg{mutex_};
    if (window_) {
      KillTimer(window_, cleanup_timer_id_);
      DestroyWindow(window_);
      window_ = {};
    }
    assert(!is_running());
  }

  bool is_running() noexcept
  {
    const std::lock_guard lg{mutex_};
    return static_cast<bool>(window_);
  }

  void send(const HWND window, const ipc::Response& response)
  {
    const std::lock_guard lg{mutex_};
    send__(window, response);
  }

  [[nodiscard]] std::future<std::unique_ptr<ipc::Response>>
  send(const HWND window, const ipc::Request& request)
  {
    const std::lock_guard lg{mutex_};
    send__(window, request);
    return (pending_responses_[request.id()] = Pending_response{
      std::chrono::steady_clock::now(),
      window,
      std::promise<std::unique_ptr<ipc::Response>>{}}).promise.get_future();
  }

private:
  struct Pending_response final {
    std::chrono::time_point<std::chrono::steady_clock> creation_time;
    HWND responder{};
    std::promise<std::unique_ptr<ipc::Response>> promise;
  };

  Handler handler_;
  std::wstring clss_;
  HINSTANCE instance_;
  constexpr static const UINT_PTR cleanup_timer_id_{1};

  std::mutex mutex_;
  HWND window_;
  bool is_running_{};
  std::unordered_map<std::int64_t, Pending_response> pending_responses_;

  static ATOM register_window(const HINSTANCE instance, const std::wstring& clss)
  {
    WNDCLASSEXW wcex{};
    wcex.cbSize      = sizeof(WNDCLASSEX);
    wcex.style       = 0;
    wcex.lpfnWndProc = wndproc;
    wcex.cbClsExtra  = 0;
    wcex.cbWndExtra  = 0;
    wcex.hInstance   = instance;
    wcex.hIcon       = nullptr;
    wcex.hCursor       = nullptr;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName  = nullptr;
    wcex.lpszClassName = clss.c_str();
    wcex.hIconSm       = nullptr;
    return RegisterClassExW(&wcex);
  }

  static LRESULT CALLBACK wndproc(const HWND window,
    const UINT message, const WPARAM wparam, const LPARAM lparam)
  {
    static const auto instance = [](const HWND window)noexcept
    {
      return reinterpret_cast<Messenger*>(GetWindowLongPtr(window, GWLP_USERDATA));
    };

    switch (message) {
    case WM_CREATE:
      {
        const auto* const self = static_cast<Messenger*>(
          reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
      }
      break;
    case WM_COPYDATA:
      {
        auto* const self = instance(window);
        const auto sender = reinterpret_cast<HWND>(wparam);
        const auto* const cds = reinterpret_cast<COPYDATASTRUCT*>(lparam);
        auto response = self->handler_(sender, std::string_view{
          static_cast<char*>(cds->lpData),
          static_cast<std::string_view::size_type>(cds->cbData)}, cds->dwData);
        if (response) {
          const std::lock_guard lg{self->mutex_};
          if (const auto it = self->pending_responses_.find(response->id());
            it != self->pending_responses_.cend() && it->second.responder == sender) {
            /*
             * We can't assert it because we can get the pending response too late -
             * after the promise is removed from self->pending_responses_ by WM_TIMER.
             */
            if (const auto* const error = dynamic_cast<ipc::Error*>(response.get())) {
              try {
                error->throw_from_this();
              } catch (...) {
                try {
                  it->second.promise.set_exception(std::current_exception());
                } catch (...){
                  assert(false);
                }
              }
            } else
              it->second.promise.set_value(std::move(response));
            self->pending_responses_.erase(it);
          }
        }
      }
      return true;
    case WM_TIMER:
      if (wparam == cleanup_timer_id_) {
        auto* const self = instance(window);
        const std::lock_guard lg{self->mutex_};
        while (true) {
          const auto it = find_if(self->pending_responses_.begin(),
            self->pending_responses_.end(),
            [now = std::chrono::steady_clock::now()](const auto& pr)
            {
              return now - pr.second.creation_time > std::chrono::minutes{1};
            });
          if (it != self->pending_responses_.cend())
            self->pending_responses_.erase(it);
          else
            break;
        }
      }
      break;
    case WM_DESTROY:
      {
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        PostQuitMessage(0);
      }
      break;
    default:
      return -1;
    }
    return 0;
  }

  void send__(const HWND recipient, const ipc::Message& message)
  {
    if (!window_)
      throw std::runtime_error{"cannot send message: ipc::wm::Messenger not running"};
    else if (!message.id())
      throw std::runtime_error{"cannot send message: invalid message identifier"};

    auto [fmt, data] = message.to_serialized();
    COPYDATASTRUCT cds{};
    cds.dwData = static_cast<ULONG_PTR>(fmt);
    cds.cbData = static_cast<DWORD>(data.size());
    cds.lpData = static_cast<PVOID>(data.data());
    SendMessage(recipient, WM_COPYDATA,
      reinterpret_cast<WPARAM>(window_),
      reinterpret_cast<LPARAM>(static_cast<LPVOID>(&cds)));
    if (const auto err = GetLastError())
      throw std::runtime_error{system_message(err)};
  }
};

} // namespace dmitigr::winbase::ipc::wm
