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

#include "../base/assert.hpp"
#include "../net/socket.hpp"
#include "connection.hpp"
#include "copier.hpp"
#include "exceptions.hpp"
#include "large_object.hpp"
#include "ready_for_query.hpp"
#include "statement.hpp"

#include <iostream>

namespace dmitigr::pgfe {

namespace detail {

/// A wrapper around net::poll().
inline Socket_readiness poll_sock(const int socket, const Socket_readiness mask,
  const std::optional<std::chrono::milliseconds> timeout)
{
  using Sock = net::Socket_native;
  using Sock_readiness = net::Socket_readiness;
  return static_cast<Socket_readiness>(net::poll(
      static_cast<Sock>(socket),
      static_cast<Sock_readiness>(mask),
      timeout ? *timeout : std::chrono::milliseconds{-1}));
}

} // namespace detail

DMITIGR_PGFE_INLINE Server_status ping(const Connection_options& options)
{
  const detail::pq::Connection_options opts{options};
  constexpr int expand_dbname{};
  const auto result = PQpingParams(opts.keywords(), opts.values(), expand_dbname);
  switch (result) {
  case PQPING_OK:
    return Server_status::ready;
  case PQPING_REJECT:
    return Server_status::unready;
  case PQPING_NO_RESPONSE:
    return Server_status::unavailable;
  case PQPING_NO_ATTEMPT:
    throw Generic_exception{"due to client-side problem no attempt was made to"
      " contact the PostgreSQL server"};
  default:;
  }
  DMITIGR_ASSERT(false);
  throw false; // disable -Wreturn-type
}

// =============================================================================

DMITIGR_PGFE_INLINE Connection::Request::Request(const Id id) noexcept
  : id_{id}
{}

DMITIGR_PGFE_INLINE Connection::Request::Request(const Id id,
  Prepared_statement prepared_statement) noexcept
  : id_{id}
  , prepared_statement_{std::move(prepared_statement)}
{}

DMITIGR_PGFE_INLINE Connection::Request::Request(const Id id,
  std::string prepared_statement_name) noexcept
  : id_{id}
  , prepared_statement_name_{std::move(prepared_statement_name)}
{}

// =============================================================================

DMITIGR_PGFE_INLINE Connection::~Connection() noexcept
{
  reset_session();
}

// General private constructor.
DMITIGR_PGFE_INLINE Connection::Connection(void* const loop, Options options)
  : options_{std::move(options)}
  , execute_ps_state_{std::make_shared<Prepared_statement::State>("", this)}
#ifdef DMITIGR_LIBS_AIO_ASIO
  , loop_{static_cast<DMITIGR_LIBS_AIO_ASIO_NAMESPACE::io_context*>(loop)}
#endif
{}

#ifdef DMITIGR_LIBS_AIO_ASIO
DMITIGR_PGFE_INLINE Connection::Connection(
  Must_be_shared_ptr,
  DMITIGR_LIBS_AIO_ASIO_NAMESPACE::io_context& loop,
  Options options)
  : Connection{&loop, std::move(options)}
{}
#endif

DMITIGR_PGFE_INLINE Connection::Connection(Must_be_shared_ptr, Options options)
  : Connection{nullptr, std::move(options)}
{}

DMITIGR_PGFE_INLINE const Connection_options& Connection::options() const noexcept
{
  return options_;
}

DMITIGR_PGFE_INLINE bool Connection::is_ssl_secured() const noexcept
{
  return conn() ? PQsslInUse(conn()) : false;
}

DMITIGR_PGFE_INLINE auto Connection::status() const noexcept -> Status
{
  if (polling_status_) {
    DMITIGR_ASSERT(conn());
    return *polling_status_;
  } else if (conn()) {
    return (PQstatus(conn()) == CONNECTION_OK) ? Status::connected :
      Status::failure;
  } else
    return Status::disconnected;
}

DMITIGR_PGFE_INLINE bool Connection::is_connected() const noexcept
{
  return status() == Status::connected;
}

DMITIGR_PGFE_INLINE bool Connection::is_connected_and_idle() const noexcept
{
  const auto ts = transaction_status();
  return ts && ts != Transaction_status::active;
}

DMITIGR_PGFE_INLINE std::optional<Transaction_status>
Connection::transaction_status() const noexcept
{
  if (is_connected()) {
    switch (PQtransactionStatus(conn())) {
    case PQTRANS_IDLE:
      return Transaction_status::unstarted;
    case PQTRANS_ACTIVE:
      return Transaction_status::active;
    case PQTRANS_INTRANS:
      return Transaction_status::uncommitted;
    case PQTRANS_INERROR:
      return Transaction_status::failed;
    default:
      return std::nullopt;
    }
  } else
    return std::nullopt;
}

DMITIGR_PGFE_INLINE bool Connection::is_transaction_uncommitted() const noexcept
{
  return (transaction_status() == Transaction_status::uncommitted);
}

DMITIGR_PGFE_INLINE std::int_fast32_t Connection::server_pid() const noexcept
{
  return is_connected() ? PQbackendPID(conn()) : 0;
}

DMITIGR_PGFE_INLINE std::optional<std::chrono::system_clock::time_point>
Connection::session_start_time() const noexcept
{
  return session_start_time_;
}

DMITIGR_PGFE_INLINE auto Connection::connect_nio() -> Status
{
  if (const auto s = status(); s == Status::connected) {
    return s;
  } else if (s == Status::establishment_reading ||
    s == Status::establishment_writing) {
    DMITIGR_ASSERT(conn());
    switch (PQconnectPoll(conn())) {
    case PGRES_POLLING_READING:
      polling_status_ = Status::establishment_reading;
      DMITIGR_ASSERT(status() == Status::establishment_reading);
      goto done;

    case PGRES_POLLING_WRITING:
      polling_status_ = Status::establishment_writing;
      DMITIGR_ASSERT(status() == Status::establishment_writing);
      goto done;

    case PGRES_POLLING_FAILED:
      polling_status_.reset();
      DMITIGR_ASSERT(status() == Status::failure);
      goto done;

    case PGRES_POLLING_OK:
      polling_status_.reset();
      session_start_time_ = std::chrono::system_clock::now();
      /*
       * We cannot assert here that status() is "connected", because it can
       * become "failure" at *any* time, even just after successful connection
       * establishment!
       */
      DMITIGR_ASSERT(status() == Status::connected || status() == Status::failure);
      goto done;

    default:
      DMITIGR_ASSERT(false);
    } // switch
  } else /* failure or disconnected */ {
    if (s == Status::failure)
      disconnect();

    DMITIGR_ASSERT(status() == Status::disconnected);

    const detail::pq::Connection_options pq_options{options_};
    constexpr int expand_dbname{};
    conn_.reset(PQconnectStartParams(pq_options.keywords(),
        pq_options.values(), expand_dbname));
    if (conn_) {
      const auto conn_status = PQstatus(conn());
      if (conn_status == CONNECTION_BAD)
        throw Generic_exception{error_message()};
      else
        polling_status_ = Status::establishment_writing;

      // Caution: until now we cannot use status()!
      DMITIGR_ASSERT(socket() >= 0);
      DMITIGR_ASSERT(status() == Status::establishment_writing);

      PQsetNoticeReceiver(conn(), &notice_receiver, this);
    } else
      throw std::bad_alloc{};
  }

 done:
  assert(is_invariant_ok());
  return status();
}

// -----------------------------------------------------------------------------
// AIO
// -----------------------------------------------------------------------------

#ifdef DMITIGR_PGFE_AIO

DMITIGR_PGFE_INLINE void
Connection::aio_handle(const Err& err, const Request& req) noexcept
{
  try {
    if (const auto& handler = req.response_handler_)
      handler(err, *this);
  } catch (const dmitigr::Exception& e) {
    std::clog << "AIO handler: " << e.err().message() << '\n';
  } catch (const std::exception& e) {
    std::clog << "AIO handler: " << e.what() << '\n';
  } catch (...) {
    std::clog << "AIO handler: unknown error\n";
  }
}

DMITIGR_PGFE_INLINE void
Connection::aio_disconnect_and_handle(const Err& err) noexcept
{
  dismiss_request(); // update last_processed_request_
  Request req;
  std::swap(req, last_processed_request_);
  disconnect(); // resets last_processed_request_
  aio_handle(err, req);
  DMITIGR_ASSERT(!is_connected());
}

DMITIGR_PGFE_INLINE void
Connection::aio_set_establishment_handler(const Asio_ip_tcp::socket::wait_type v)
{
  loop_sock().async_wait(v, [this](const std::error_code& ec)
  {
    try {
      if (ec) {
        return connect_handler_(Err{ec.default_error_condition()}, *this);
      } else if (const auto s = connect_nio(); s == Status::failure) {
        return connect_handler_(Err{Errc::generic, "AIO connect failure"}, *this);
      } else if (s == Status::establishment_reading) {
        aio_set_establishment_handler(Asio_ip_tcp::socket::wait_read);
      } else if (s == Status::establishment_writing) {
        aio_set_establishment_handler(Asio_ip_tcp::socket::wait_write);
      } else if (s == Status::connected) {
        connect_handler_(Err{Errc::success}, *this);
        connect_handler_ = {};
        aio_set_connected_read_ready_handler();
      }
    } catch (...) {
      disconnect();
    }
  });
}

DMITIGR_PGFE_INLINE void Connection::aio_set_connected_read_ready_handler()
{
  if (!is_connected())
    return;

  loop_sock().async_wait(Asio_ip_tcp::socket::wait_read, asio_handler([this]
  {
    // std::clog << "AIO connected read handler\n";
    read_input();
    while (handle_input(false) == Response_status::ready) {
      // std::clog << "  call AIO handler\n";
      const auto& req = response_.status() == PGRES_SINGLE_TUPLE ?
        requests_.front() : last_processed_request_;
      DMITIGR_ASSERT(req.response_handler_);
      aio_handle(Err{}, req);
    }
    aio_set_connected_read_ready_handler();
    if (has_uncompleted_request())
      aio_flush_output();
  }));
}

DMITIGR_PGFE_INLINE void Connection::aio_set_connected_write_ready_handler()
{
  if (!is_connected() || is_output_flushed())
    return;

  loop_sock().async_wait(Asio_ip_tcp::socket::wait_write, asio_handler([this]
  {
    // std::clog << "AIO connected write handler\n";
    aio_flush_output();
  }));
}

DMITIGR_PGFE_INLINE void Connection::connect_aio(Aio_handler connect_handler,
  std::optional<std::chrono::milliseconds> timeout)
{
  if (!loop_)
    throw Generic_exception{"cannot initiate connection establishment:"
      " no AIO loop provided"};
  else if (!connect_handler)
    throw Generic_exception{"cannot initiate connection establishment:"
      " invalid connect handler provided"};

#ifdef DMITIGR_LIBS_AIO_ASIO
  using Tcp = Asio_ip_tcp;
  try {
    const auto s = connect_nio();
    DMITIGR_ASSERT(s == Status::establishment_writing);
  } catch (const dmitigr::Exception& e) {
    connect_handler(e.err(), *this);
  } catch (...) {
    connect_handler(Err{Errc::generic}, *this);
  }
  set_nio_output_enabled(true);
  loop_sock_ = Tcp::socket{loop_->get_executor(), Tcp::v4(), socket()};
  auto& sock = *loop_sock_;
  sock.non_blocking(true);
  sock.native_non_blocking(true);
  DMITIGR_ASSERT(sock.native_non_blocking());
  connect_handler_ = std::move(connect_handler);
  aio_set_establishment_handler(Tcp::socket::wait_write);
#else
#error Unsupported AIO type
#endif
}
#endif  // DMITIGR_PGFE_AIO

DMITIGR_PGFE_INLINE void
Connection::connect(std::optional<std::chrono::milliseconds> timeout)
{
  using std::chrono::milliseconds;
  using std::chrono::system_clock;
  using std::chrono::duration_cast;

  if (!(!timeout || timeout >= milliseconds{-1}))
    throw Generic_exception{"cannot initiate connection: invalid timeout "
      "specified"};

  if (is_connected())
    return; // No need to check invariant. Just return.

  if (timeout == milliseconds{-1})
    timeout = options().connect_timeout();

  const auto is_timeout = [&timeout]
  {
    return timeout <= milliseconds::zero();
  };
  static const auto throw_timeout = []
  {
    throw Generic_exception{Errc::timed_out, "connection timeout"};
  };

  // Stage 1: beginning.
  auto timepoint1 = system_clock::now();
  auto current_status = connect_nio();

  if (timeout) {
    *timeout -= duration_cast<milliseconds>(system_clock::now() - timepoint1);
    if (is_timeout())
      throw_timeout();
  }

  // Stage 2: polling.
  while (current_status != Status::connected) {
    timepoint1 = system_clock::now();

    Socket_readiness current_socket_readiness{};
    switch (current_status) {
    case Status::establishment_reading:
      current_socket_readiness = wait_socket_readiness(
        Socket_readiness::read_ready, timeout);
      break;

    case Status::establishment_writing:
      current_socket_readiness = wait_socket_readiness(
        Socket_readiness::write_ready, timeout);
      break;

    case Status::connected:
      break;

    case Status::disconnected:
      DMITIGR_ASSERT(false);

    case Status::failure:
      throw Generic_exception{error_message()};
    }

    if (timeout) {
      *timeout -= duration_cast<milliseconds>(system_clock::now() - timepoint1);
      if (is_timeout()) {
        DMITIGR_ASSERT(current_socket_readiness == Socket_readiness::unready);
        (void)current_socket_readiness;
        throw_timeout();
      }
    }

    current_status = connect_nio();
  } // while

  DMITIGR_ASSERT(status() == Status::connected);
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE void Connection::disconnect() noexcept
{
  reset_session();
  conn_.reset(); // discarding unhandled notifications btw.
  DMITIGR_ASSERT(status() == Status::disconnected);
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Socket_readiness
Connection::wait_socket_readiness(Socket_readiness mask,
  std::optional<std::chrono::milliseconds> timeout) const
{
  using std::chrono::system_clock;
  using std::chrono::milliseconds;
  using std::chrono::duration_cast;

  if (!(!timeout || timeout >= milliseconds{-1}))
    throw Generic_exception{"cannot wait socket readiness: "
      "invalid timeout specified"};
  else if (!!bool(loop()))
    throw Generic_exception{"cannot wait socket readiness: "
      "already observed by AIO loop"};
  else if (!((status() != Status::failure) && (status() != Status::disconnected)))
    throw Generic_exception{"cannot wait socket readiness: "
      "invalid connection status"};

  DMITIGR_ASSERT(socket() >= 0);

  while (true) {
    const auto timepoint1 = system_clock::now();
    try {
      return detail::poll_sock(socket(), mask, timeout);
    } catch (const std::system_error& e) {
      // Retry on EINTR.
      if (e.code() == std::errc::interrupted) {
        if (timeout) {
          *timeout -= duration_cast<milliseconds>(system_clock::now() - timepoint1);
          if (timeout <= milliseconds::zero())
            // Timeout.
            return Socket_readiness::unready;
        } else
          continue;
      } else
        throw;
    }
  }
}

DMITIGR_PGFE_INLINE Socket_readiness
Connection::socket_readiness(const Socket_readiness mask) const
{
  constexpr std::chrono::milliseconds no_wait_just_poll{};
  return wait_socket_readiness(mask, no_wait_just_poll);
}

DMITIGR_PGFE_INLINE void Connection::read_input()
{
  if (!PQconsumeInput(conn()))
    throw Generic_exception{error_message()};
}

/*
 * According to https://www.postgresql.org/docs/current/libpq-async.html,
 * "PQgetResult() must be called repeatedly until it returns a null pointer,
 * indicating that the command is done."
 */
DMITIGR_PGFE_INLINE Response_status
Connection::handle_input(const bool wait_response)
{
  if (!is_connected())
    throw Generic_exception{"cannot handle input from server: not connected"};

  const auto check_execute_in_progress = [this]() noexcept
  {
    DMITIGR_ASSERT(response_status_ == Response_status::ready);
    DMITIGR_ASSERT(response_.status() == PGRES_SINGLE_TUPLE);
    DMITIGR_ASSERT(!requests_.empty());
    DMITIGR_ASSERT(requests_.front().id_ == Request::Id::execute);
  };

  const auto is_completion_status = [this](const auto status) noexcept
  {
    return status == PGRES_FATAL_ERROR ||
#ifdef LIBPQ_HAS_PIPELINING
      status == PGRES_PIPELINE_ABORTED ||
      status == PGRES_PIPELINE_SYNC ||
#endif
      status == PGRES_COMMAND_OK ||
      status == PGRES_TUPLES_OK ||
      status == PGRES_EMPTY_QUERY ||
      status == PGRES_BAD_RESPONSE;
  };

  const auto is_response_can_be_replaced_with_next_one = [this,
    &is_completion_status]() noexcept
  {
    return response_status_ == Response_status::ready &&
      is_completion_status(response_.status());
  };

  /*
   * According to https://www.postgresql.org/docs/current/libpq-pipeline-mode.html,
   * "To enter single-row mode, call PQsetSingleRowMode() before retrieving
   * results with PQgetResult(). This mode selection is effective only for the
   * query currently being processed." Therefore, set_single_row_mode_enabled()
   * is called once for each query in a pipeline.
   */
  if ((pipeline_status() == Pipeline_status::enabled) &&
    !is_single_row_mode_enabled_ && !requests_.empty() &&
    requests_.front().id_ == Request::Id::execute)
    set_single_row_mode_enabled();

  if (wait_response) {
    if (response_status_ == Response_status::unready) {
    complete_response:
      while (auto* const r = PQgetResult(conn()))
        PQclear(r);
      response_status_ = Response_status::ready_not_preprocessed;
      dismiss_request();
    } else if (!response_ || is_response_can_be_replaced_with_next_one()) {
      response_.reset(PQgetResult(conn()));
      if (response_.status() == PGRES_SINGLE_TUPLE) {
        response_status_ = Response_status::ready;
        check_execute_in_progress();
        goto handle_notifications;
      } else if (is_completion_status(response_.status()))
        goto complete_response;
      else if (response_)
        response_status_ = Response_status::ready_not_preprocessed;
      else
        response_status_ = Response_status::empty;
    }
  } else {
    /*
     * Checks for nonblocking result and handles notices btw.
     * @remark: notice_receiver() (which calls the notice handler) will be
     * called indirectly from PQisBusy().
     * @remark: PQisBusy() calls a routine (pqParseInput3() from
     * fe-protocol3.c) which parses consumed input and stores notifications and
     * notices if are available. (PQnotifies() calls this routine as well.)
     */
    static const auto is_get_result_would_block = [](PGconn* const conn) noexcept
    {
      return PQisBusy(conn) == 1;
    };

    if (response_status_ == Response_status::unready) {
    try_complete_response:
      while (!is_get_result_would_block(conn())) {
        if (auto* const r = PQgetResult(conn()); !r) {
          response_status_ = Response_status::ready_not_preprocessed;
          dismiss_request();
          break;
        } else
          PQclear(r);
      }
    } else if (!response_ || is_response_can_be_replaced_with_next_one()) {
      if (!is_get_result_would_block(conn())) {
        response_.reset(PQgetResult(conn()));
        if (response_.status() == PGRES_SINGLE_TUPLE) {
          response_status_ = Response_status::ready;
          check_execute_in_progress();
          goto handle_notifications;
        } else if (is_completion_status(response_.status())) {
          response_status_ = Response_status::unready;
          goto try_complete_response;
        } else if (response_)
          response_status_ = Response_status::ready_not_preprocessed;
        else
          response_status_ = Response_status::empty;
      }
    }
  }

  // Preprocessing the response_. (This is done only once for response_!)
  if (response_status_ == Response_status::ready_not_preprocessed) {
    const auto rstatus = response_.status();
    DMITIGR_ASSERT(rstatus != PGRES_NONFATAL_ERROR);
    DMITIGR_ASSERT(rstatus != PGRES_SINGLE_TUPLE);
    if (rstatus == PGRES_TUPLES_OK) {
      DMITIGR_ASSERT(last_processed_request_.id_ == Request::Id::execute);
      is_single_row_mode_enabled_ = false;
    } else if (rstatus == PGRES_COPY_OUT || rstatus == PGRES_COPY_IN) {
      // is_copy_in_progress() now returns `true`, copier() returns Copier.
      copier_state_ = std::make_shared<Connection*>(nullptr); // can throw
    } else if (rstatus == PGRES_FATAL_ERROR) {
      // is_copy_in_progress() now returns `false`.
      reset_copier_state();
      is_single_row_mode_enabled_ = false;
    } else if (rstatus == PGRES_COMMAND_OK) {
      auto& lpr = last_processed_request_;
      DMITIGR_ASSERT(lpr.id_ != Request::Id::prepare || lpr.prepared_statement_);
      DMITIGR_ASSERT(lpr.id_ != Request::Id::describe || lpr.prepared_statement_);
      DMITIGR_ASSERT(lpr.id_ != Request::Id::unprepare || lpr.prepared_statement_name_);
      if (lpr.id_ == Request::Id::prepare) {
        auto& ps = lpr.prepared_statement_;
        DMITIGR_ASSERT(ps);
        const auto [p, e] = registered_ps(ps.name());
        DMITIGR_ASSERT(p == e);
        register_ps(std::move(ps)); // can throw (ps will not be affected)
        DMITIGR_ASSERT(last_prepared_statement_);
      } else if (lpr.id_ == Request::Id::describe) {
        auto& ps = lpr.prepared_statement_;
        DMITIGR_ASSERT(ps);
        auto response = release_response();
        try {
          ps.set_description(std::move(response)); // can throw
        } catch (...) {
          // response is not yet moved in this case, so it can be reset back.
          reset_response(std::move(response));
          throw;
        }
        register_ps(std::move(ps)); // can throw (ps will not be affected)
        DMITIGR_ASSERT(last_prepared_statement_);
      } else if (lpr.id_ == Request::Id::unprepare) {
        DMITIGR_ASSERT(lpr.prepared_statement_name_ &&
          !std::strcmp(response_.command_tag(), "DEALLOCATE"));
        unregister_ps(*lpr.prepared_statement_name_);
      }
      // is_copy_in_progress() now returns `false`.
      reset_copier_state();
      is_single_row_mode_enabled_ = false;
    }
    response_status_ = Response_status::ready;
  }

 handle_notifications:
  try {
    // Note: notifications are collected by PQisBusy() and PQgetResult().
    if (notification_handler_) {
      while (auto* const n = PQnotifies(conn()))
        notification_handler_(Notification{n});
    }
  } catch (const std::exception& e) {
    std::clog << "notification handler: error: " << e.what() << '\n';
  } catch (...) {
    std::clog << "notification handler: unknown error\n";
  }

  assert(is_invariant_ok());
  return response_status_;
}

DMITIGR_PGFE_INLINE void Connection::set_nio_output_enabled(const bool value)
{
  if (PQsetnonblocking(conn(), value))
    throw Generic_exception{"cannot set nonblocking output mode on connection"};
}

DMITIGR_PGFE_INLINE bool Connection::is_nio_output_enabled() const noexcept
{
  return PQisnonblocking(conn());
}

DMITIGR_PGFE_INLINE bool Connection::flush_output(const bool wait)
{
  if (!is_nio_output_enabled())
    throw Generic_exception{"cannot flush queued output data to the server: "
      "nonblocking output mode is disabled on connection"};

  if (const int r{PQflush(conn())}; r == 1) {
    if (wait) {
      using Sr = Socket_readiness;
      const auto sr = wait_socket_readiness(Sr::read_ready | Sr::write_ready);
      if (sr == Sr::read_ready) {
        read_input();
        return flush_output(wait);
      } else if (sr == Sr::write_ready)
        return flush_output(wait);
    } else
      return is_output_flushed_ = false;
  } else if (!r)
    return is_output_flushed_ = true;

  throw Generic_exception{"cannot flush queued output data to the server"};
}

DMITIGR_PGFE_INLINE bool Connection::is_output_flushed() const noexcept
{
  return is_output_flushed_;
}

DMITIGR_PGFE_INLINE bool
Connection::wait_response(std::optional<std::chrono::milliseconds> timeout)
{
  using std::chrono::system_clock;
  using std::chrono::milliseconds;
  using std::chrono::duration_cast;

  if (!(is_connected() && has_uncompleted_request()))
    return false;

  if (!(!timeout || timeout >= milliseconds{-1}))
    throw Generic_exception{"cannot wait response: invalid timeout specified"};

  if (timeout < milliseconds::zero()) // even if timeout < -1
    timeout = options().wait_response_timeout();

  Response_status rs{Response_status::unready};
  if (timeout) {
    read_input();
    rs = handle_input(false);
    while (rs != Response_status::ready) {
      const auto moment_of_wait = system_clock::now();
      if (wait_socket_readiness(
          Socket_readiness::read_ready, timeout) == Socket_readiness::read_ready) {
        timeout = *timeout - duration_cast<milliseconds>(system_clock::now() - moment_of_wait);
        read_input();
        rs = handle_input(false);
      } else // timeout expired
        throw Generic_exception{Errc::timed_out, "wait response timeout expired"};
    }
  } else
    rs = handle_input(true);

  return rs == Response_status::ready;
}

DMITIGR_PGFE_INLINE bool
Connection::wait_response_throw(const std::optional<std::chrono::milliseconds> timeout)
{
  const bool result = wait_response(timeout);
  throw_if_error();
  return result;
}

DMITIGR_PGFE_INLINE Error Connection::error() noexcept
{
  return (response_.status() == PGRES_FATAL_ERROR) ?
    Error{release_response()} : Error{};
}

DMITIGR_PGFE_INLINE Row Connection::row() noexcept
{
  return (response_.status() == PGRES_SINGLE_TUPLE)
    ? Row{release_response()} : Row{};
}

DMITIGR_PGFE_INLINE Notification Connection::pop_notification()
{
  auto* const n = PQnotifies(conn());
  return n ? Notification{n} : Notification{};
}

DMITIGR_PGFE_INLINE void
Connection::set_notice_handler(Notice_handler handler)
{
  notice_handler_ = std::move(handler);
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE auto Connection::notice_handler() const noexcept
  -> const Notice_handler&
{
  return notice_handler_;
}

DMITIGR_PGFE_INLINE void
Connection::set_notification_handler(Notification_handler handler)
{
  notification_handler_ = std::move(handler);
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE auto
Connection::notification_handler() const noexcept -> const Notification_handler&
{
  return notification_handler_;
}

DMITIGR_PGFE_INLINE bool Connection::has_response() const noexcept
{
  return static_cast<bool>(response_) && (response_status_ == Response_status::ready);
}

DMITIGR_PGFE_INLINE Copier Connection::copier() noexcept
{
  const auto s = response_.status();
  return (s == PGRES_COPY_IN || s == PGRES_COPY_OUT) && !*copier_state_ ?
    Copier{*this, release_response()} : Copier{};
}

DMITIGR_PGFE_INLINE Completion Connection::completion() noexcept
{
  switch (response_.status()) {
  case PGRES_TUPLES_OK:
    return Completion{release_response().command_tag()};
  case PGRES_COMMAND_OK:
    switch (last_processed_request_.id_) {
    case Request::Id::execute:
      return Completion{release_response().command_tag()};
    case Request::Id::prepare:
      [[fallthrough]];
    case Request::Id::describe:
      return {};
    case Request::Id::unprepare:
      release_response();
      return Completion{"unprepare"};
    default:
      DMITIGR_ASSERT(false);
    }
  case PGRES_EMPTY_QUERY:
    return Completion{""};
  case PGRES_BAD_RESPONSE:
    return Completion{"invalid"};
  default:
    return {};
  }
}

DMITIGR_PGFE_INLINE Prepared_statement
Connection::prepared_statement() noexcept
{
  release_response();
  return std::move(last_prepared_statement_);
}

DMITIGR_PGFE_INLINE Ready_for_query Connection::ready_for_query() noexcept
{
#ifdef LIBPQ_HAS_PIPELINING
  return (response_.status() == PGRES_PIPELINE_SYNC)
    ? Ready_for_query{release_response()} : Ready_for_query{};
#else
  return Ready_for_query{};
#endif
}

DMITIGR_PGFE_INLINE bool Connection::is_ready_for_request() const noexcept
{
  return (pipeline_status() == Pipeline_status::disabled) &&
    is_connected_and_idle();
}

DMITIGR_PGFE_INLINE bool Connection::is_ready_for_nio_request() const noexcept
{
  return (pipeline_status() == Pipeline_status::disabled) ?
    is_ready_for_request() : is_connected();
}

DMITIGR_PGFE_INLINE bool Connection::is_copy_in_progress() const noexcept
{
  return static_cast<bool>(copier_state_);
}

DMITIGR_PGFE_INLINE std::size_t Connection::request_queue_size() const noexcept
{
  return requests_.size();
}

DMITIGR_PGFE_INLINE bool Connection::has_uncompleted_request() const noexcept
{
  return !requests_.empty();
}

DMITIGR_PGFE_INLINE void
Connection::prepare_nio(const Statement& statement, const std::string& name)
{
  prepare_nio__(statement.to_query_string(*this).c_str(),
    name.c_str(), &statement); // can throw
}

DMITIGR_PGFE_INLINE void
Connection::prepare_nio_as_is(const std::string& statement, const std::string& name)
{
  prepare_nio__(statement.c_str(), name.c_str(), nullptr); // can throw
}

#ifdef DMITIGR_PGFE_AIO
DMITIGR_PGFE_INLINE void Connection::prepare_aio(Aio_handler handler,
  const Statement& statement, const std::string& name)
{
  prepare_describe_aio__(std::move(handler), &statement, name);
}

DMITIGR_PGFE_INLINE void Connection::describe_aio(Aio_handler handler,
  const std::string& name)
{
  prepare_describe_aio__(std::move(handler), nullptr, name);
}

DMITIGR_PGFE_INLINE void Connection::prepare_describe_aio__(Aio_handler handler,
  const Statement* const statement, const std::string& name)
{
  if (!loop())
    throw Generic_exception{"cannot prepare/describe statement: invalid AIO loop"};
  else if (!handler)
    throw Generic_exception{"cannot prepare/describe statement: AIO handler is empty"};
  if (statement)
    prepare_nio(*statement, name);
  else
    describe_nio(name);
  DMITIGR_ASSERT(!requests_.empty());
  requests_.front().response_handler_ = std::move(handler);
  aio_flush_output();
}
#endif

DMITIGR_PGFE_INLINE Prepared_statement
Connection::prepare(const Statement& statement, const std::string& name)
{
  using M = void(Connection::*)(const Statement&, const std::string&);
  return prepare__(static_cast<M>(&Connection::prepare_nio), statement, name);
}

DMITIGR_PGFE_INLINE Prepared_statement
Connection::prepare_as_is(const std::string& statement, const std::string& name)
{
  return prepare__(&Connection::prepare_nio_as_is, statement, name);
}

DMITIGR_PGFE_INLINE void Connection::describe_nio(const std::string& name)
{
  if (!is_ready_for_nio_request())
    throw Generic_exception{"cannot describe prepared statement: "
      "not ready for non-blocking IO request"};

  const auto [p, e] = registered_ps(name);
  auto state = (p == e) ?
    std::make_shared<Prepared_statement::State>(name, this) : *p;
  Prepared_statement ps{state};
  requests_.emplace(Request::Id::describe, std::move(ps)); // can throw
  try {
    const int send_ok = PQsendDescribePrepared(conn(), name.c_str());
    if (!send_ok)
      throw Generic_exception{error_message()};
  } catch (...) {
    requests_.pop(); // rollback
    throw;
  }

  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Prepared_statement Connection::describe(const std::string& name)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot describe prepared statement: "
      "not ready for non-blocking IO request"};
  describe_nio(name);
  return wait_prepared_statement__();
}

DMITIGR_PGFE_INLINE void Connection::unprepare_nio(const std::string& name)
{
  if (name.empty())
    throw Generic_exception{"cannot unprepare prepared statement: "
      "invalid name specified"};

  auto name_copy = name; // can throw
  const auto query = "DEALLOCATE " + to_quoted_identifier(name); // can throw
  execute_nio(query); // can throw
  DMITIGR_ASSERT(requests_.front().id_ == Request::Id::execute);
  requests_.front().id_ = Request::Id::unprepare; // cannot throw
  requests_.front().prepared_statement_name_ = std::move(name_copy); // cannot throw

  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Completion Connection::unprepare(const std::string& name)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot unprepare prepared statement: "
      "not ready for request"};
  unprepare_nio(name);
  wait_response_throw();
  return completion();
}

DMITIGR_PGFE_INLINE void Connection::set_pipeline_enabled(const bool value)
{
#ifdef LIBPQ_HAS_PIPELINING
  if (value) {
    if (!PQenterPipelineMode(conn()))
      throw Generic_exception{"cannot enable pipeline on connection"};
  } else {
    if (!PQexitPipelineMode(conn()))
      throw Generic_exception{error_message()};
  }
#else
  throw Generic_exception{std::string{"cannot "}
    .append(value ? "enable" : "disable")
    .append(" pipeline: feature is not available")};
#endif
}

DMITIGR_PGFE_INLINE Pipeline_status Connection::pipeline_status() const noexcept
{
#ifdef LIBPQ_HAS_PIPELINING
  switch (PQpipelineStatus(conn())) {
  case PQ_PIPELINE_OFF:
    return Pipeline_status::disabled;
  case PQ_PIPELINE_ON:
    return Pipeline_status::enabled;
  case PQ_PIPELINE_ABORTED:
    return Pipeline_status::aborted;
  default:
    break;
  }
  DMITIGR_ASSERT(false);
#else
  return Pipeline_status::disabled;
#endif
}

DMITIGR_PGFE_INLINE void Connection::send_sync()
{
#ifdef LIBPQ_HAS_PIPELINING
  if (!PQpipelineSync(conn()))
    throw Generic_exception{"cannot send sync message to the server"};
  requests_.emplace(Request::Id::sync);
#else
  throw Generic_exception{"cannot send sync message: feature is not available"};
#endif
}

DMITIGR_PGFE_INLINE void Connection::send_flush()
{
#ifdef LIBPQ_HAS_PIPELINING
  if (!PQsendFlushRequest(conn()))
    throw Generic_exception{"cannot send flush message to the server"};
#else
  throw Generic_exception{"cannot send flush message: feature is not available"};
#endif
}

DMITIGR_PGFE_INLINE void
Connection::set_result_format(const Data_format format)
{
  default_result_format_ = format;
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Data_format Connection::result_format() const noexcept
{
  return default_result_format_;
}

DMITIGR_PGFE_INLINE Oid Connection::create_large_object(const Oid oid)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot create large object: not ready for request"};
  const Oid result = (oid == invalid_oid) ?
    lo_creat(conn(),
      static_cast<int>(
        Large_object_open_mode::reading |
        Large_object_open_mode::writing)) :
    lo_create(conn(), oid);
  if (result == invalid_oid)
    throw Generic_exception{"cannot create large object: "+error_message()};
  return result;
}

DMITIGR_PGFE_INLINE Large_object Connection::open_large_object(const Oid oid,
  const Large_object_open_mode mode)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot open large object: not ready for request"};

  const int desc{lo_open(conn(), oid, static_cast<int>(mode))};
  if (desc < 0)
    throw Generic_exception{"cannot open large object: "+error_message()};

  Large_object result{std::make_shared<Large_object::State>(++lo_id_, desc, this)};
  register_lo(result);
  return result;
}

DMITIGR_PGFE_INLINE void Connection::remove_large_object(const Oid oid)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot remove large object: not ready for request"};

  if (lo_unlink(conn(), oid) == -1)
    throw Generic_exception{"cannot remove large object: "+error_message()};
}

DMITIGR_PGFE_INLINE Oid
Connection::import_large_object(const std::filesystem::path& filename,
  const Oid oid)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot import large object: not ready for request"};

  const Oid result = lo_import_with_oid(conn(), filename.string().c_str(), oid);
  if (result == invalid_oid)
    throw Generic_exception{"cannot import large object: "+error_message()};

  return result;
}

DMITIGR_PGFE_INLINE void
Connection::export_large_object(const Oid oid,
  const std::filesystem::path& filename)
{
  if (!is_ready_for_request())
    throw Generic_exception{"cannot export large object: not ready for request"};

  if (lo_export(conn(), oid, filename.string().c_str()) == -1)
    throw Generic_exception{"cannot export large object: "+error_message()};
}

DMITIGR_PGFE_INLINE std::string
Connection::to_quoted_literal(const std::string_view literal) const
{
  if (!is_connected())
    throw Generic_exception{"cannot quote literal: not connected"};

  using Uptr = std::unique_ptr<char, void(*)(void*)>;
  if (const auto p = Uptr{PQescapeLiteral(conn(), literal.data(),
        literal.size()), &PQfreemem})
    return p.get();
  else if (is_out_of_memory())
    throw std::bad_alloc{};
  else
    throw Generic_exception{error_message()};
}

DMITIGR_PGFE_INLINE std::string
Connection::to_quoted_identifier(const std::string_view identifier) const
{
  if (!is_connected())
    throw Generic_exception{"cannot quote identifier: not connected"};

  using Uptr = std::unique_ptr<char, void(*)(void*)>;
  if (const auto p = Uptr{PQescapeIdentifier(conn(), identifier.data(),
        identifier.size()), &PQfreemem})
    return p.get();
  else if (is_out_of_memory())
    throw std::bad_alloc{};
  else
    throw Generic_exception{error_message()};
}

DMITIGR_PGFE_INLINE std::unique_ptr<Data>
Connection::to_hex_data(const Data& data) const
{
  auto [storage, size] = to_hex_storage(data);
  return Data::make(std::move(storage), size, Data_format::text);
}

// -----------------------------------------------------------------------------
// private
// -----------------------------------------------------------------------------

DMITIGR_PGFE_INLINE bool Connection::is_invariant_ok() const noexcept
{
  const bool conn_ok = conn_ || !polling_status_;
  const bool polling_status_ok =
    !polling_status_ ||
    (*polling_status_ == Status::establishment_reading) ||
    (*polling_status_ == Status::establishment_writing);
  const bool requests_ok = !is_connected() || is_ready_for_nio_request() ||
    !requests_.empty();
  const bool session_start_time_ok =
    (status() == Status::connected) == static_cast<bool>(session_start_time_);
  const bool session_data_empty =
    !session_start_time_ &&
    !response_ &&
    (response_status_ == Response_status::empty) &&
    ps_states_.empty() &&
    lo_states_.empty() &&
    requests_.empty();
  const bool session_data_ok = session_data_empty ||
    (status() == Status::failure) || (status() == Status::connected);
  const bool trans_ok = !is_connected() || transaction_status();
  const bool sess_time_ok = !is_connected() || session_start_time();
  const bool pid_ok = !is_connected() || server_pid();
  const bool readiness_ok = is_ready_for_nio_request() || !is_ready_for_request();

  // std::clog << conn_ok << " "
  //           << polling_status_ok << " "
  //           << requests_ok << " "
  //           << session_start_time_ok << " "
  //           << session_data_ok << " "
  //           << trans_ok << " "
  //           << sess_time_ok << " "
  //           << pid_ok << " "
  //           << readiness_ok << " "
  //           << std::endl;

  return
    conn_ok &&
    polling_status_ok &&
    requests_ok &&
    session_start_time_ok &&
    session_data_ok &&
    trans_ok &&
    sess_time_ok &&
    pid_ok &&
    readiness_ok;
}

DMITIGR_PGFE_INLINE detail::pq::Result Connection::release_response() noexcept
{
  response_status_ = Response_status::empty;
  return std::move(response_);
}

DMITIGR_PGFE_INLINE void
Connection::reset_response(detail::pq::Result&& response) noexcept
{
  if ( (response_ = std::move(response)))
    response_status_ = Response_status::ready;
}

DMITIGR_PGFE_INLINE void Connection::reset_session() noexcept
{
#ifdef DMITIGR_PGFE_AIO
  connect_handler_ = {};
  loop_sock_.reset();
#endif
  session_start_time_.reset();
  response_.reset();
  response_status_ = {};
  requests_ = {};
  last_processed_request_ = {};
  is_output_flushed_ = true;
  reset_copier_state();
  is_single_row_mode_enabled_ = false;

  // Reset prepared statements.
  last_prepared_statement_ = {};
  for (auto& s : ps_states_) {
    DMITIGR_ASSERT(s);
    s->connection_ = nullptr;
  }
  ps_states_.clear();

  // Reset large objects.
  for (auto& s : lo_states_) {
    DMITIGR_ASSERT(s);
    s->connection_ = nullptr;
  }
  lo_states_.clear();
}

DMITIGR_PGFE_INLINE void Connection::reset_copier_state() noexcept
{
  if (copier_state_) {
    *copier_state_ = nullptr;
    copier_state_ = {};
  }
}

DMITIGR_PGFE_INLINE void Connection::set_single_row_mode_enabled() noexcept
{
  const auto set_ok = PQsetSingleRowMode(conn());
  DMITIGR_ASSERT(set_ok);
  is_single_row_mode_enabled_ = true;
}

DMITIGR_PGFE_INLINE void
Connection::notice_receiver(void* const arg, const PGresult* const r) noexcept
{
  DMITIGR_ASSERT(arg);
  DMITIGR_ASSERT(r);
  auto* const cn = static_cast<Connection*>(arg);
  if (cn->notice_handler_) {
    try {
      cn->notice_handler_(Notice{r});
    } catch (const std::exception& e) {
      std::clog << "notice handler: error: " << e.what() << '\n';
    } catch (...) {
      std::clog << "notice handler: unknown error\n";
    }
  }
}

DMITIGR_PGFE_INLINE void
Connection::default_notice_handler(const Notice& n) noexcept
{
  std::clog << "PostgreSQL Notice: " << n.brief() << '\n';
}

DMITIGR_PGFE_INLINE void
Connection::prepare_nio__(const char* const query, const char* const name,
  const Statement* const preparsed)
{
  if (!is_ready_for_nio_request())
    throw Generic_exception{"cannot prepare statement: "
      "not ready for non-blocking IO request"};
  DMITIGR_ASSERT(query);
  DMITIGR_ASSERT(name);

  auto state = std::make_shared<Prepared_statement::State>(name, this);
  Prepared_statement ps{std::move(state), preparsed, true};
  requests_.emplace(Request::Id::prepare, std::move(ps));
  try {
    constexpr int n_params{};
    constexpr const ::Oid* const param_types{};
    const int send_ok{PQsendPrepare(conn(), name, query, n_params, param_types)};
    if (!send_ok)
      throw Generic_exception{error_message()};
  } catch (...) {
    requests_.pop(); // rollback
    throw;
  }

  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Prepared_statement Connection::wait_prepared_statement__()
{
  wait_response_throw();
  if (auto comp = completion()) {
    DMITIGR_ASSERT(comp.tag() == "invalid");
    throw Generic_exception{Errc::invalid_response};
  }
  return prepared_statement();
}

DMITIGR_PGFE_INLINE void
Connection::register_ps(Prepared_statement&& ps)
{
  if (const auto [p, e] = registered_ps(ps.name()); p == e)
    ps_states_.push_back(ps.state_); // can throw
  last_prepared_statement_ = std::move(ps);
  DMITIGR_ASSERT(last_prepared_statement_);
}

DMITIGR_PGFE_INLINE void
Connection::unregister_ps(const std::string_view name) noexcept
{
  const auto [p, e] = registered_ps(name);
  (void)e;
  unregister_ps(p);
}

DMITIGR_PGFE_INLINE void
Connection::unregister_ps(decltype(ps_states_)::const_iterator p) noexcept
{
  unregister(ps_states_, p);
}

DMITIGR_PGFE_INLINE int Connection::socket() const noexcept
{
  return PQsocket(conn());
}

DMITIGR_PGFE_INLINE void Connection::throw_if_error()
{
  if (auto err = error())
    throw Sqlstate_exception{std::make_shared<Error>(std::move(err))};
}

DMITIGR_PGFE_INLINE Completion&& Connection::completion_or_throw(Completion&& comp)
{
  if (comp.tag() == "invalid")
    throw Generic_exception{Errc::invalid_response};
  else
    return std::move(comp);
}

DMITIGR_PGFE_INLINE std::string Connection::error_message() const
{
  /*
   * If nullptr passed to PQerrorMessage() it returns
   * something like "connection pointer is NULL\n".
   */
  return conn() ? str::value_or_empty(PQerrorMessage(conn())) : std::string{};
}

DMITIGR_PGFE_INLINE bool Connection::is_out_of_memory() const noexcept
{
  constexpr const char msg[] = "out of memory";
  return !std::strncmp(PQerrorMessage(conn()), msg, sizeof(msg) - 1);
}

DMITIGR_PGFE_INLINE std::pair<std::unique_ptr<void, void(*)(void*)>, std::size_t>
Connection::to_hex_storage(const Data& data) const
{
  if (!is_connected())
    throw Generic_exception{"cannot encode data to hex: not connected"};
  else if (!(data && (data.format() == Data_format::binary)))
    throw Generic_exception{"cannot encode data to hex: invalid data specified"};

  const auto from_length = data.size();
  const auto* const from = static_cast<const unsigned char*>(data.bytes());
  std::size_t result_length{};
  using Uptr = std::unique_ptr<void, void(*)(void*)>;
  if (auto storage = Uptr{PQescapeByteaConn(conn(), from, from_length,
        &result_length), &PQfreemem})
    // The result_length includes the terminating zero byte of the result.
    return std::make_pair(std::move(storage), result_length - 1);
  else
    /*
     * Currently, the only possible error is insufficient memory for the result
     * string. See: https://www.postgresql.org/docs/current/static/libpq-exec.html#LIBPQ-PQESCAPEBYTEACONN
     */
    throw std::bad_alloc{};
}

DMITIGR_PGFE_INLINE void Connection::register_lo(const Large_object& lo)
{
  lo_states_.push_back(lo.state_);
}

DMITIGR_PGFE_INLINE void Connection::unregister_lo(Large_object& lo) noexcept
{
  const auto [l, e] = registered_lo(lo.state_->id_);
  (void)e;
  unregister(lo_states_, l);
}

DMITIGR_PGFE_INLINE void
Connection::unregister_lo(decltype(lo_states_)::const_iterator l) noexcept
{
  DMITIGR_ASSERT(l != end(lo_states_));
  unregister(lo_states_, l);
}

DMITIGR_PGFE_INLINE bool Connection::close(Large_object& lo) noexcept
{
  unregister_lo(lo);
  return lo_close(conn(), lo.descriptor()) == 0;
}

DMITIGR_PGFE_INLINE std::int_fast64_t Connection::seek(Large_object& lo,
  std::int_fast64_t offset, Large_object_seek_whence whence)
{
  const auto result{lo_lseek64(conn(), lo.descriptor(), offset,
    static_cast<int>(whence))};
  if (result == -1)
    throw Generic_exception{"cannot seek large object: "+error_message()};
  return result;
}

DMITIGR_PGFE_INLINE std::int_fast64_t Connection::tell(Large_object& lo)
{
  const auto result{lo_tell64(conn(), lo.descriptor())};
  if (result == -1)
    throw Generic_exception{"cannot tell large object: "+error_message()};
  return result;
}

DMITIGR_PGFE_INLINE void Connection::truncate(Large_object& lo,
  const std::int_fast64_t new_size)
{
  const int result{lo_truncate64(conn(), lo.descriptor(),
    static_cast<pg_int64>(new_size))};
  if (result == -1)
    throw Generic_exception{"cannot truncate large object: "+error_message()};
}

DMITIGR_PGFE_INLINE int Connection::read(Large_object& lo, char* const buf,
  const std::size_t size)
{
  const int result{lo_read(conn(), lo.descriptor(), buf, size)};
  if (result == -1)
    throw Generic_exception{"cannot read large object: "+error_message()};
  return result;
}

DMITIGR_PGFE_INLINE int Connection::write(Large_object& lo, const char* const buf,
  const std::size_t size)
{
  const int result{lo_write(conn(), lo.descriptor(), buf, size)};
  if (result == -1)
    throw Generic_exception{"cannot write large object: "+error_message()};
  return result;
}

} // namespace dmitigr::pgfe
