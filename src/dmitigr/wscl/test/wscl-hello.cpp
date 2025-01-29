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

#include "../../base/assert.hpp"
#include "../../wscl/wscl.hpp"

#include <chrono>
#include <iostream>

namespace chrono = std::chrono;
namespace wscl = dmitigr::wscl;

class Connection final : public wscl::Connection {
public:
  using wscl::Connection::Connection;

private:
  void handle_open() noexcept override
  {
    DMITIGR_ASSERT(is_open());
    std::cout << "Connection open!" << std::endl;
    send_utf8("Hello from dmitigr::wscl!");
  }

  void handle_message(const std::string_view data,
    const wscl::Data_format format) noexcept override
  {
    std::cout << "Received: ";
    if (format == wscl::Data_format::binary)
      std::cout << "binary data of size " << data.size();
    else
      std::cout << data;
    std::cout << std::endl;
  }

  void handle_error(const int code, const std::string_view message) noexcept override
  {
    DMITIGR_ASSERT(!is_open());
    std::cout << "Connection error: " << code << " (" << message << ")" << std::endl;
  }

  void handle_close(const int code, const std::string_view reason) noexcept override
  {
    DMITIGR_ASSERT(!is_open());
    std::cout << "Connection closed: " << code << " (" << reason << ")" << std::endl;
  }
};

#ifdef UWSC_USE_UV
void handle_signal(uv_signal_t* const sig, const int /*signum*/) noexcept
#else
void handle_signal(struct ev_loop* const loop, ev_signal* const sig, const int /*revents*/) noexcept
#endif
{
  if (sig->signum == SIGINT) {
#ifdef UWSC_USE_UV
    uv_stop(sig->loop);
#else
    ev_break(loop, EVBREAK_ALL);
#endif
    std::clog << "Graceful shutdown by SIGINT." << std::endl;
  }
}

Connection make_connection(uwsc_loop* const loop)
{
  return Connection{loop, wscl::Connection_options{}
    .set_host("localhost")
    .set_port(9001)
    .set_ping_interval(chrono::seconds{10})};
}

int main()
{
  try {
#ifdef UWSC_USE_UV
    auto* const loop = uv_default_loop();
    auto conn = make_connection(loop);
    uv_signal_t signal_watcher;
    uv_signal_init(loop, &signal_watcher);
    uv_signal_start(&signal_watcher, &handle_signal, SIGINT);
    uv_update_time(loop);
    uv_run(loop, UV_RUN_DEFAULT);
#else
    auto* const loop = EV_DEFAULT;
    auto conn = make_connection(loop);
    ev_signal signal_watcher;
    ev_signal_init(&signal_watcher, &handle_signal, SIGINT);
    ev_signal_start(loop, &signal_watcher);
    ev_run(loop, 0);
#endif
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "unknown error\n";
    return 1;
  }
}
