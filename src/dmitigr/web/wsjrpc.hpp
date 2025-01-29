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

#ifndef DMITIGR_WEB_WSJRPC_HPP
#define DMITIGR_WEB_WSJRPC_HPP

#include "../base/log.hpp"
#include "../jrpc/jrpc.hpp"
#include "../ws/ws.hpp"
#include "exceptions.hpp"

namespace dmitigr::web {

class Ws_jrpc_connection : public dmitigr::ws::Connection {
protected:
  using Super = dmitigr::ws::Connection;
  using Super::send_utf8;
  using Super::send_binary;
  using Jerrc = jrpc::Server_errc;

  // ---------------------------------------------------------------------------
  // ws::Connection overrides
  // ---------------------------------------------------------------------------

  void handle_message(const std::string_view payload,
    const dmitigr::ws::Data_format format) noexcept override
  {
    try {
      if (format != ws::Data_format::utf8) {
        // Ignore binary format request.
        log::clog() << "wsjrpc::handle_message: binary format request ignored\n";
        return;
      }

      std::uint_fast64_t id{};
      try {
        const auto req = jrpc::Request::from_json(payload);
        if (!req.id()) {
          // Ignore notification.
          log::clog() << "wsjrpc::handle_message: notification ignored\n";
          return;
        } else if (!req.id()->IsUint() && !req.id()->IsUint64())
          req.throw_error(Jerrc::invalid_request,
            "non-unsigned integer IDs are forbidden");
        else if (id = req.id()->GetUint64(); !(id > 0))
          req.throw_error(Jerrc::invalid_request,
            "ID is not positive integer");

        if (is_ready()) {
          const auto result = call(req);
          send_utf8(result);
        } else
          req.throw_error(Errc::service_not_ready,
            "service is temporarily unavailable");
      } catch(const jrpc::Error& e) {
        log::clog() << "wsjrpc::handle_message: ";
        std::string what = e.what();
        const bool what_was_empty = what.empty();
        if (what_was_empty)
          what = jrpc::to_literal_anyway(jrpc::Server_errc{e.condition().value()});
        std::clog << what << '\n';
        const auto& err = what_was_empty ? jrpc::Error{e.condition(), id, what} : e;
        send_utf8(err);
      } catch(const dmitigr::Exception& e) {
        std::string what = e.what();
        if (what.empty())
          what = e.err().message();
        const jrpc::Error err{e.condition(), id, what};
        log::clog() << "wsjrpc::handle_message: " << what << '\n';
        send_utf8(err);
      } catch(const std::logic_error& e) {
        std::string what = e.what();
        if (what.empty())
          what = "bug";
        const jrpc::Error err{Jerrc::internal_error, id, what};
        log::clog() << "wsjrpc::handle_message: bug: " << what << '\n';
        send_utf8(err);
      } catch(const std::exception& e) {
        std::string what = e.what();
        if (what.empty())
          what = "generic internal error";
        const jrpc::Error err{Jerrc::internal_error, id, what};
        log::clog() << "wsjrpc::handle_message: internal error: " << what << '\n';
        send_utf8(err);
      } catch(...) {
        const jrpc::Error err{Jerrc::internal_error, id, "unknown internal error"};
        log::clog() << "wsjrpc::handle_message: internal error: unknown error\n";
        send_utf8(err);
      }
    } catch(const std::exception& e) {
      log::clog() << "wsjrpc::handle_message: " << e.what() << '\n';
    } catch(...) {
      log::clog() << "wsjrpc::handle_message: unknown error\n";
    }
  }

  // ---------------------------------------------------------------------------
  // RPC methods
  // ---------------------------------------------------------------------------

  virtual jrpc::Result call(const jrpc::Request& req)
  {
    if (const auto method = req.method(); method == "ping")
      return ping(req);
    else
      req.throw_error(Jerrc::method_not_found);
  }

  virtual jrpc::Result ping(const jrpc::Request& req)
  {
    return req.make_result("pong");
  }

  // ---------------------------------------------------------------------------
  // Normal methods
  // ---------------------------------------------------------------------------

  virtual bool is_ready() const noexcept
  {
    return true;
  }

  void send_utf8(const jrpc::Response& res)
  {
    send_utf8(res.to_string());
  }
};

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_WSJRPC_HPP
