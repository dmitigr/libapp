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

#include "../base/assert.hpp"
#include "../http/errc.hpp"
#include "exceptions.hpp"
#include "http_io.hpp"
#include "util.hpp"
#include "uwebsockets.hpp"

#include <mutex>
#include <string>

namespace dmitigr::ws::detail {

class iHttp_io : public Http_io {
  template<bool> friend class iHttp_io_templ;

  iHttp_io() = default;
};

template<bool IsSsl>
class iHttp_io_templ final : public iHttp_io {
public:
  iHttp_io_templ(uWS::HttpResponse<IsSsl>* const rep, Server* const server)
    : rep_{rep}
    , server_{server}
  {
    DMITIGR_ASSERT(rep_ && server_);
  }

  bool is_valid__() const noexcept
  {
    return static_cast<bool>(rep_);
  }

  bool is_valid() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return is_valid__();
  }

  bool loop_submit(std::function<void()> callback) noexcept override
  {
    const std::lock_guard lg{mut_};
    if (is_valid__()) {
      us_loop_t* const uss_loop = us_socket_context_loop(IsSsl, ctx__());
      auto* const loop = reinterpret_cast<uWS::Loop*>(uss_loop);
      loop->defer(std::move(callback));
      return true;
    } else
      return false;
  }

  const Server* server() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return is_valid__() ? server_ : nullptr;
  }

  Server* server() noexcept override
  {
    return const_cast<Server*>(
      static_cast<const iHttp_io_templ*>(this)->server());
  }

  Connection* connection__() const noexcept
  {
    return ws_data_.conn.get();
  }

  const Connection* connection() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return connection__();
  }

  Connection* connection() noexcept override
  {
    return const_cast<Connection*>(
      static_cast<const iHttp_io_templ*>(this)->connection());
  }

  void end_handshake() override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__() || !connection__())
      throw Exception{"cannot end WebSocket handshake: invalid HTTP I/O"};

    rep_->upgrade(std::move(ws_data_), sec_ws_key_, sec_ws_protocol_,
      sec_ws_extensions_, ctx__());
    rep_ = nullptr;
    sec_ws_key_ = sec_ws_protocol_ = sec_ws_extensions_ = {};
    DMITIGR_ASSERT(!is_valid__());
    DMITIGR_ASSERT(!connection__());
  }

  void send_status(const http::Server_errc code) override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot send HTTP status line: invalid HTTP I/O"};

    rep_->writeStatus(std::to_string(static_cast<int>(code)).append(" ")
      .append(to_literal_anyway(code)));
  }

  void send_header(const std::string_view name, const std::string_view value) override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot send HTTP header: invalid HTTP I/O"};

    rep_->writeHeader(name, value);
  }

  std::pair<bool, bool> send_content(const std::string_view data,
    const std::uintmax_t total_size) override
  {
    static_assert(sizeof(decltype(data.size())) <= sizeof(decltype(total_size)));
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot send HTTP content: invalid HTTP I/O"};

    if (!is_send_handler_set_) {
      end__(data);
      return {true, true};
    } else
      return rep_->tryEnd(data, total_size);
  }

  void end__(const std::string_view data)
  {
    if (!is_valid__())
      throw Exception{"cannot send HTTP content: invalid HTTP I/O"};

    rep_->end(data);
    DMITIGR_ASSERT(rep_->hasResponded());
    rep_ = nullptr;
    DMITIGR_ASSERT(!is_valid__());
  }

  void end(const std::string_view data = {}) override
  {
    const std::lock_guard lg{mut_};
    end__(data);
  }

  void set_send_handler(Send_handler handler) override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot set HTTP send handler: invalid HTTP I/O"};
    else if (is_send_handler_set_)
      throw Exception{"cannot set HTTP send handler: already set"};
    else if (!handler)
      throw Exception{"cannot set invalid HTTP send handler"};

    rep_->onWritable([this, handler = std::move(handler)](const std::uintmax_t pos)
    {
      const std::lock_guard lg{mut_};
      DMITIGR_ASSERT(rep_);
      const auto ok = handler(pos);
      if (ok) {
        rep_ = nullptr;
        DMITIGR_ASSERT(!is_valid__());
      }
      return ok;
    });
    is_send_handler_set_ = true;
  }

  bool is_send_handler_set() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return is_send_handler_set_;
  }

  void abort() noexcept override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      return;
    rep_->close();
    rep_ = nullptr;
    DMITIGR_ASSERT(!is_valid__());
  }

  void set_abort_handler(Abort_handler handler) override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot set HTTP abort handler: invalid HTTP I/O"};
    else if (is_abort_handler_set_)
      throw Exception{"cannot set HTTP abort handler: already set"};
    else if (!handler)
      throw Exception{"cannot set invalid HTTP abort handler"};

    rep_->onAborted([this, handler = std::move(handler)]
    {
      const std::lock_guard lg{mut_};
      handler();
      rep_ = nullptr;
      DMITIGR_ASSERT(!is_valid__());
    });
    is_abort_handler_set_ = true;
  }

  bool is_abort_handler_set() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return is_abort_handler_set_;
  }

  void set_receive_handler(Receive_handler handler) override
  {
    const std::lock_guard lg{mut_};
    if (!is_valid__())
      throw Exception{"cannot set HTTP receive handler: invalid HTTP I/O"};
    else if (is_receive_handler_set_)
      throw Exception{"cannot set HTTP receive handler: already set"};
    else if (!handler)
      throw Exception{"cannot set invalid HTTP receive handler"};

    rep_->onData(std::move(handler));
    is_receive_handler_set_ = true;
  }

  bool is_receive_handler_set() const noexcept override
  {
    const std::lock_guard lg{mut_};
    return is_receive_handler_set_;
  }

private:
  template<bool> friend class detail::Srv;

  mutable std::mutex mut_;
  bool is_abort_handler_set_{};
  bool is_send_handler_set_{};
  bool is_receive_handler_set_{};
  uWS::HttpResponse<IsSsl>* rep_{}; // reinterpretable as us_socket_t*
  Server* server_{};
  Ws_data ws_data_;
  std::string sec_ws_key_;
  std::string sec_ws_protocol_;
  std::string sec_ws_extensions_;

  us_socket_context_t* ctx__()
  {
    auto* const uss = reinterpret_cast<us_socket_t*>(rep_);
    return us_socket_context(IsSsl, uss);
  };
};

} // namespace dmitigr::ws::detail
