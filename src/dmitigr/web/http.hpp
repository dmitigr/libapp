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

#ifndef DMITIGR_WEB_HTTP_HPP
#define DMITIGR_WEB_HTTP_HPP

#include "../base/assert.hpp"
#include "../base/thread.hpp"
#include "../http/cookie.hpp"
#include "../http/errc.hpp"
#include "../http/errctg.hpp"
#include "../base/log.hpp"
#include "../jrpc/jrpc.hpp"
#include "../net/address.hpp"
#include "../str/sequence.hpp"
#include "../str/stream.hpp"
#include "../str/transform.hpp"
#include "../tpl/generic.hpp"
#include "../url/query_string.hpp"
#include "../ws/http_io.hpp"
#include "../ws/http_request.hpp"
#include "basics.hpp"
#include "config.hpp"
#include "lisp.hpp"
#include "exceptions.hpp"
#include "util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <shared_mutex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::web {

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

/// @returns `true` if `reqpath` is valid.
inline bool is_valid_request_path(const std::string_view reqpath) noexcept
{
  return !reqpath.empty() &&
    reqpath.front() == '/' &&
    std::all_of(cbegin(reqpath) + 1, cend(reqpath), [](const unsigned char c)
    {
      return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '/';
    });
}

/// The vector of pairs of extensions with counterparts.
inline const std::vector<std::pair<std::string_view, std::string_view>> tpl_extensions{
  {".html", ".thtml"},
  {".css", ".tcss"},
  {".js", ".tjs"},
  {".json", ".tjson"},
  {".xml", ".txml"},
  {".txt", ".ttxt"}
};

/// @returns `true` if `ext` represents a template filename extension.
inline bool is_tpl_extension(const std::filesystem::path& ext) noexcept
{
  return !ext.empty() && any_of(cbegin(tpl_extensions), cend(tpl_extensions),
    [&ext](const auto& x){return ext == x.second;});
}

/**
 * @returns Template filename extension, promoted from `ext`, or empty path if
 * no such a promotion possible.
 */
inline std::filesystem::path to_tpl_extension(const std::filesystem::path& ext)
{
  using std::filesystem::path;
  if (ext.empty())
    return ext;
  const auto b = cbegin(tpl_extensions);
  const auto e = cend(tpl_extensions);
  const auto i = find_if(b, e, [&ext](const auto& x){return ext == x.first;});
  return i != e ? path{i->second} : path{};
}

/// @returns The value of the `Content-Type` header by `fname`.
inline std::string_view content_type(const std::filesystem::path& fname) noexcept
{
  if (const auto ext = fname.extension(); ext.empty())
    return "application/octet-stream";
  else if (ext == ".html" || ext == ".thtml")
    return "text/html";
  else if (ext == ".css" || ext == ".tcss")
    return "text/css";
  else if (ext == ".js" || ext == ".tjs")
    return "application/javascript";
  else if (ext == ".json" || ext == ".tjson")
    return "application/json";
  else if (ext == ".xml" || ext == ".txml")
    return "text/xml";
  else if (ext == ".txt" || ext == ".ttxt")
    return "text/plain";
  else if (ext == ".ico")
    return "image/x-icon";
  else if (ext == ".png")
    return "image/png";
  else if (ext == ".jpeg" || ext == ".jpg")
    return "image/jpeg";
  else if (ext == ".gif")
    return "image/gif";
  else if (ext == ".svg" || ext == ".svgz")
    return "image/svg+xml";
  else if (ext == ".pdf")
    return "application/pdf";
  else if (ext == ".ods")
    return "application/vnd.oasis.opendocument.spreadsheet";
  else if (ext == "odt")
    return "application/vnd.oasis.opendocument.text";
  else if (ext == ".7z")
    return "application/x-7z-compressed";
  else if (ext == ".zip")
    return "application/zip";
  else if (ext == ".mp3")
    return "audio/mpeg";
  else if (ext == ".ogg")
    return "audio/ogg";
  else if (ext == ".m4a")
    return "audio/x-m4a";
  else if (ext == ".mp4")
    return "video/mp4";
  else if (ext == ".mpeg" || ext == ".mpg")
    return "video/mpeg";
  else if (ext == ".mov")
    return "video/quicktime";
  else if (ext == ".flv")
    return "video/x-flv";
  else if (ext == ".m4v")
    return "video/x-m4v";
  else if (ext == ".wmv")
    return "video/x-ms-wmv";
  else if (ext == ".avi")
    return "video/x-msvideo";
  else
    return "application/octet-stream";
}

// -----------------------------------------------------------------------------
// Senders
// -----------------------------------------------------------------------------

/**
 * @brief Sends the error page.
 *
 * @returns `true` on success.
 */
inline bool send_error(std::shared_ptr<ws::Http_io> io,
  const http::Server_errc status) noexcept
{
  try {
    if (!io || !io->is_valid())
      throw Exception{"cannot send error: invalid IO"};

    const auto phrase = to_literal_anyway(status);
    const auto content = std::string{"<!doctype html><html><title>"}
      .append(phrase).append("</title>")
      .append("<body><h1>").append(phrase).append("</h1></body></html>");
    io->send_status(status);
    io->send_header("Content-Type", "text/html");
    io->end(content);
    return true;
  } catch (const std::exception& e) {
    log::clog()<<"HTTP: send error: "<<e.what()<<"\n";
    return false;
  } catch (...) {
    log::clog()<<"HTTP: send error: unknown error\n";
    return false;
  }
}

/**
 * @brief Sends the data.
 *
 * @returns `true` on success.
 *
 * @todo Handle backpressure.
 */
inline bool send_data(std::shared_ptr<ws::Http_io> io,
  std::string data,
  const std::string_view content_type) noexcept
{
  try {
    if (!io)
      throw Exception{"cannot send data: invalid IO"};

    io->send_status(http::Server_errc::ok);
    io->send_header("Content-Type", content_type);
    io->end(data);
    return true;
  } catch (const std::exception& e) {
    log::clog()<<"HTTP: send data: "<<e.what()<<"\n";
    return false;
  } catch (...) {
    log::clog()<<"HTTP: send data: unknown error\n";
    return false;
  }
}

/**
 * @brief Sends the specified file.
 *
 * @return `true` on success.
 */
inline bool send_file(std::shared_ptr<ws::Http_io> io,
  const std::filesystem::path& fname,
  const bool is_attachment) noexcept
{
  try {
    if (!io)
      throw Exception{"cannot send file: invalid IO"};

    // Make the input stream.
    const auto in = std::make_shared<std::ifstream>(fname);
    if (!in->good())
      return send_error(io, http::Server_errc::internal_server_error);

    // Get the file size.
    using std::ios_base;
    in->seekg(0, ios_base::end);
    const auto file_size = in->tellg();
    in->seekg(0, ios_base::beg);

    constexpr std::size_t send_buffer_size{128 * 1024};
    io->set_send_handler([io, in, file_size](const std::uintmax_t pos) -> bool
    {
      // badbit shouldn't be possible here, but check it just in case.
      if (in->bad()) {
        log::clog()<<"HTTP: send file: badbit input file stream\n";
        io->abort();
        return true;
      }

      /*
       * failbit is possible when the stream is done (std::basic_ios::eof() is
       * `true`) but not all the data sent (the last call of Http_io::send_data()
       * returned `{false, false}`).
       */
      in->clear();

      in->seekg(pos, std::ios_base::beg);
      while (true) {
        char buf[send_buffer_size];
        in->read(buf, sizeof(buf));
        const auto gcount =
          static_cast<std::string_view::size_type>(in->gcount());
        const auto [ok, done] =
          io->send_content(std::string_view{buf, gcount}, file_size);
        if (!ok || done)
          return ok;
      }
      DMITIGR_ASSERT(false);
    });

    // Send headers.
    io->send_header("Content-Type", content_type(fname));
    if (is_attachment)
      io->send_header("Content-Disposition", std::string{"attachment; filename="}
        .append(fname.filename().string()));

    // Send the initial part of content.
    while (true) {
      char buf[send_buffer_size];
      in->read(buf, sizeof(buf));
      const auto gcount =
        static_cast<std::string_view::size_type>(in->gcount());
      const auto [ok, done] =
        io->send_content(std::string_view{buf, gcount}, file_size);
      if (!ok || done)
        break;
    }
    return true;
  } catch (const std::exception& e) {
    log::clog()<<"HTTP: send file: "<<e.what()<<"\n";
    return false;
  } catch (...) {
    log::clog()<<"HTTP: send file: unknown error\n";
    return false;
  }
}

// -----------------------------------------------------------------------------
// HTTP request handler
// -----------------------------------------------------------------------------

/// HTTP request handler (httper).
class Httper final : public std::enable_shared_from_this<Httper> {
public:
  /// HTTP request data.
  struct Request final {
    /// `0` means guest account.
    std::int_fast64_t account_id{};
    /**
     * Extracted from "language" cookie, or from "x-default-language" header,
     * or from Httper::default_language().
     */
    Language language;
    std::string method;
    std::string path;
    std::smatch path_smatch;
    std::string body;
    std::string content_type;
    std::filesystem::path filepath;
    std::filesystem::path filename;
    url::Query_string query_string;
    http::Cookie cookie;
    /**
     * Extracted directly from HTTP request, or from "x-remote-ip-address"
     * header if Httper::is_behind_proxy().
     */
    net::Ip_address remote_ip_address;
  };

  /// The alias of a text template handler (tpler).
  using Tpler = std::function<void(tpl::Generic&, const Request&)>;

  /// The alias of a RPC handler (rpcer).
  using Rpcer = std::function<jrpc::Result(const jrpc::Request&, const Request&)>;

  /// The alias of a generic handler (gener).
  using Gener = std::function<void(std::shared_ptr<ws::Http_io>, const Request&)>;

  /// The alias of a `{regex, tpler}` pair.
  using Regex_tpler_pair = std::pair<std::regex, Tpler>;

  /// The alias of a `{regex, rpcer}` pair.
  using Regex_rpcer_pair = std::pair<std::regex, Rpcer>;

  /**
   * @brief The alias of the function which is called if the current request
   * path doesn't matches to any value of publics().
   *
   * @returns An account ID if request contains the valid authentication token.
   * If the returned value is `0` (guest) then the Auth_prompter will be called.
   *
   * @see Auth_prompter, publics().
   */
  using Auth_checker = std::function<std::int_fast64_t(const Request&)>;

  /**
   * @brief The alias of the function which generates the prompt for
   * authentication.
   *
   * @returns The HTML with the authentication prompt. If the returned value is
   * empty then the HTTP error 403 will be responded instead the authentication
   * prompt.
   *
   * @details Authentication prompt, in case of success, must add the valid
   * authentication info (token) to the Cookie which will be passed to the
   * Auth_checker on each following HTTP requests later.
   *
   * @see Auth_checker.
   */
  using Auth_prompter = std::function<std::string()>;

  /// Not copy-constructible.
  Httper(const Httper&) = delete;

  /// Not copy-assignable.
  Httper& operator=(const Httper&) = delete;

  /// Not move-constructible.
  Httper(Httper&&) = delete;

  /// Not move-assignable.
  Httper& operator=(Httper&&) = delete;

  /// Constructs the instance.
  static std::shared_ptr<Httper>
  make(std::shared_ptr<thread::Pool> thread_pool, const Config& cfg)
  {
    return std::shared_ptr<Httper>(new Httper{std::move(thread_pool), cfg});
  }

  // ---------------------------------------------------------------------------

  /// @returns The shared mutex which protects the data of this instance.
  const std::shared_mutex& mutex() const noexcept
  {
    return mutex_;
  }

  /// @overload
  std::shared_mutex& mutex() noexcept
  {
    return mutex_;
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns Path on the server filesystem.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const std::filesystem::path& docroot() const noexcept
  {
    return docroot_;
  }

  /**
   * @brief Sets docroot.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_docroot(std::filesystem::path value)
  {
    docroot_ = std::move(value);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /**
   * @brief Publicly available request paths.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const std::vector<std::regex>& publics() const noexcept
  {
    return publics_;
  }

  /**
   * @brief Adds public.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& add_public(std::regex value)
  {
    publics_.push_back(std::move(value));
    return *this;
  }

  /// @overload
  Httper& add_public(const std::string& value)
  {
    using re = std::regex;
    publics_.emplace_back(value, re::ECMAScript|re::icase|re::optimize);
    return *this;
  }

  /**
   * @brief Clears publics.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  void clear_publics() noexcept
  {
    publics_.clear();
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The thread pool used.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const std::shared_ptr<thread::Pool>& thread_pool() const noexcept
  {
    return thread_pool_;
  }

  // ---------------------------------------------------------------------------

  /// @name Authentication
  /// @{

  /**
   * @returns The authentication checker.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const Auth_checker& auth_checker() const noexcept
  {
    return auth_checker_;
  }

  /**
   * @brief Sets the authentication checker.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_auth_checker(Auth_checker checker)
  {
    auth_checker_ = std::move(checker);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The authentication prompter.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const Auth_prompter& auth_prompter() const noexcept
  {
    return auth_prompter_;
  }

  /**
   * @brief Sets the authentication prompter.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_auth_prompter(Auth_prompter prompter)
  {
    auth_prompter_ = std::move(prompter);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /// @}

  /// @name Generators
  /// @{

  /**
   * @returns The tpler which is invoked before the tpler found in tplers().
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const Tpler& before_tpler() const noexcept
  {
    return before_tpler_;
  }

  /**
   * @brief Sets before tpler.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_before_tpler(Tpler tpler)
  {
    before_tpler_ = std::move(tpler);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns Available templaters.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const std::vector<Regex_tpler_pair>& tplers() const noexcept
  {
    return tplers_;
  }

  /**
   * @brief Adds templater.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& add_tpler(std::regex value, Tpler tpler)
  {
    tplers_.emplace_back(std::move(value), std::move(tpler));
    return *this;
  }

  /// @overload
  Httper& add_tpler(const std::string& value, Tpler tpler)
  {
    using re = std::regex;
    tplers_.emplace_back(re{value, re::ECMAScript|re::icase|re::optimize},
      std::move(tpler));
    return *this;
  }

  /**
   * @brief Clears templaters.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  void clear_tplers() noexcept
  {
    tplers_.clear();
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns Available rpcers.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const std::vector<Regex_rpcer_pair>& rpcers() const noexcept
  {
    return rpcers_;
  }

  /**
   * @brief Adds rpcer.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& add_rpcer(std::regex value, Rpcer rpcer)
  {
    rpcers_.emplace_back(std::move(value), std::move(rpcer));
    return *this;
  }

  /// @overload
  Httper& add_rpcer(const std::string& value, Rpcer rpcer)
  {
    using re = std::regex;
    rpcers_.emplace_back(re{value, re::ECMAScript|re::icase|re::optimize},
      std::move(rpcer));
    return *this;
  }

  /**
   * @brief Clears rpcers.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  void clear_rpcers() noexcept
  {
    rpcers_.clear();
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The tpler which is invoked after the tpler found in tplers().
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const Tpler& after_tpler() const noexcept
  {
    return after_tpler_;
  }

  /**
   * @brief Sets after tpler.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_after_tpler(Tpler tpler)
  {
    after_tpler_ = std::move(tpler);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /**
   * @returns The gener which is invoked if the tpler is not found.
   *
   * @warning The mutex() must be locked before calling this function and till
   * the end of working with the returned value!
   */
  const Gener& gener() const noexcept
  {
    return gener_;
  }

  /**
   * @brief Sets gener.
   *
   * @returns *this.
   *
   * @warning The mutex() must be locked before calling this function!
   */
  Httper& set_gener(Gener gener)
  {
    gener_ = std::move(gener);
    return *this;
  }

  // ---------------------------------------------------------------------------

  /**
   * @brief Creates the template recursively (i.e. expanded).
   *
   * @details If the `tplfile` parameterized with Lisp expressions then these
   * parameters are replaced with the evaluation results of these expressions.
   *
   * @returns Expanded generic template.
   *
   * @param tplfile Either absolute or relative path to the text template file.
   */
  Ret<tpl::Generic>
  tpl(const std::filesystem::path& tplfile, const Request& req) const
  {
    // Create the Lisp environment.
    using lisp::make_expr;
    lisp::Env env;
    env.set("_lang", make_expr<lisp::Str_expr>(
        std::string{to_string_view(req.language)}))
      .set("_docroot", make_expr<lisp::Str_expr>(docroot_.generic_string()))
      .set("_tplstack", make_expr<Tplstack_expr>());
    return detail::tpl(tplfile, env);
  }

  // ---------------------------------------------------------------------------

  /// @}

  /**
   * @brief Handles the accepted HTTP connection.
   *
   * @returns `true` on success.
   *
   * @par Requires
   * `io`.
   *
   * @par Effects
   * `!io->is_valid()`.
   */
  bool handle(const ws::Http_request& request,
    std::shared_ptr<ws::Http_io> io) const noexcept
  {
    try {
      // Check the arguments.
      if (!io)
        throw Exception{"invalid HTTP I/O"};

      // Set abort handler.
      io->set_abort_handler([remote_ip = request.remote_ip_address()]
      {
        log::clog()<<"HTTP: request from "<<remote_ip.to_string()<<" aborted\n";
      });

      // Check the method.
      const auto method = str::to_uppercase(std::string{request.method()});
      if (method != "GET" && method != "POST")
        return send_error(io, http::Server_errc::method_not_allowed);

      // Check the request path.
      if (!is_valid_request_path(request.path()))
        return send_error(io, http::Server_errc::bad_request);

      // Get request info: normalized request path, filename etc.
      namespace fs = std::filesystem;
      const auto reqpathfs = fs::path{str::to_lowercase(std::string{request.path()})}
        .lexically_normal();
      auto reqpath = reqpathfs.generic_string();
      auto filepath = docroot_ / reqpathfs.relative_path();
      auto filename = filepath.filename();
      // log::clog()<<"Request path = "<<reqpath<<std::endl;

      // Maybe redirect from /path/to -> /path/to/.
      if (!filename.empty() && !filename.has_extension() && !is_regular_file(filepath)) {
        reqpath += '/';
        io->send_status(http::Server_errc::moved_permanently);
        io->send_header("Location", reqpath);
        io->end();
        return true;
      }

      // Prepare the request data.
      const auto req = std::make_shared<Request>();
      req->method = method;
      req->path = std::move(reqpath);
      req->filepath = std::move(filepath);
      req->filename = std::move(filename);
      req->cookie = http::Cookie{request.header("cookie")};
      //
      if (is_behind_proxy_) {
        const auto xria = request.header("x-remote-ip-address");
        req->remote_ip_address = !xria.empty() ?
          net::Ip_address::from_text(std::string{xria})
          : request.remote_ip_address();
      } else
        req->remote_ip_address = request.remote_ip_address();
      //
      if (method == "GET") {
        req->query_string = url::Query_string{request.query_string()};
      } else if (method == "POST") {
        req->body.reserve(64 * 1024);
        req->content_type = request.header("content-type");
      }

      // Get the language.
      std::optional<Language> lang;
      if (const auto idx = req->cookie.entry_index("language"))
        lang = to_language(req->cookie.entry(*idx).value());
      if (!lang && is_behind_proxy_)
        lang = to_language(request.header("x-default-language"));
      req->language = lang.value_or(default_language_);

      // Wrap the rest of the handler into the continuator.
      const auto continue_handle_request = [io, req, self = shared_from_this()]() -> void
      {
        const auto handle_request = [=]() -> void
        {
          try {
            // Check the authentication.
            {
              const std::shared_lock lg{self->mutex_};

              if (self->auth_checker_)
                req->account_id = self->auth_checker_(*req);

              if (!req->account_id) {
                const bool is_auth_required = none_of(cbegin(self->publics_),
                  cend(self->publics_), [req](const auto& re)
                  {
                    return regex_match(req->path, re);
                  });
                if (is_auth_required) {
                  io->loop_submit([io,
                      prompt = self->auth_prompter_ ?
                        self->auth_prompter_() : std::string{}]
                  {
                    if (!prompt.empty())
                      send_data(io, std::move(prompt), "text/html");
                    else
                      send_error(io, http::Server_errc::forbidden);
                  });
                  return;
                }
              }
            }

            // Try POST.
            if (req->method == "POST") {
              const std::shared_lock lg{self->mutex_};
              DMITIGR_ASSERT(req->body.capacity());
              if (req->content_type == "application/json") {
                const auto& rpcer = [self, req]() -> const Rpcer&
                {
                  for (const auto& p : self->rpcers_) {
                    std::smatch sm;
                    if (std::regex_match(req->path, sm, p.first)) {
                      req->path_smatch = std::move(sm);
                      return p.second;
                    }
                  }
                  return invalid_rpcer_;
                }();

                if (rpcer) {
                  const auto resbody = [req, &rpcer]
                  {
                    try {
                      return std::make_shared<std::string>(
                        rpcer(jrpc::Request::from_json(req->body), *req).to_string());
                    } catch (const jrpc::Error& e) {
                      return std::make_shared<std::string>(e.to_string());
                    }
                  }();
                  io->loop_submit([io, resbody]
                  {
                    send_data(io, std::move(*resbody), "application/json");
                  });
                } else
                  io->loop_submit([io]
                  {
                    send_error(io, http::Server_errc::not_found);
                  });
              } else
                io->loop_submit([io]
                {
                  send_error(io, http::Server_errc::bad_request);
                });
              return;
            }

            // @returns `true` if `path` is a regular file.
            const auto try_static_file = [io](auto&& path) -> bool
            {
              if (is_regular_file(path)) {
                io->loop_submit([io, path = std::move(path)]
                {
                  send_file(io, path, false);
                });
                return true;
              } else
                return false;
            };

            /*
             * @returns `true` to indicate that the request is handled. It
             * happens if either:
             *   - `path` is a template file;
             *   - there is a tpler found for the request path.
             * Returns `false` if no file found.
             */
            const auto try_template = [&](auto&& path) -> bool
            {
              const std::shared_lock lg{self->mutex_};

              const auto& tpler = [self, req]() -> const Tpler&
              {
                for (const auto& p : self->tplers_) {
                  std::smatch sm;
                  if (std::regex_match(req->path, sm, p.first)) {
                    req->path_smatch = std::move(sm);
                    return p.second;
                  }
                }
                return invalid_tpler_;
              }();

              if (auto [err, tpl] = self->tpl(path, *req); !err) {
                if (self->before_tpler_)
                  self->before_tpler_(tpl, *req);
                if (tpler)
                  tpler(tpl, *req);
                if (self->after_tpler_)
                  self->after_tpler_(tpl, *req);
                if (tpl.has_unbound_parameters()) {
                  const auto unbound_params = tpl.unbound_parameter_names();
                  throw Exception{"template for "+req->path+" has unbound parameters: "
                    +str::to_string(unbound_params, ", ")};
                }
                io->loop_submit([io, tpl = std::move(tpl), path = std::move(path)]
                {
                  if (auto [err, out] = tpl.to_output(); !err)
                    send_data(io, std::move(out), content_type(path));
                  else
                    log::clog()<<"HTTP: "<<err.message()<<"\n";
                });
                return true;
              } else if (err == Errc::file_not_found)
                return false;
              else
                throw Exception{err};
            };

            // Try the static/template files.
            if (req->filename.empty()) {
              if (try_static_file(req->filepath / "index.html") ||
                try_template(req->filepath / "index.thtml"))
                return;
            } else {
              if (const auto ext = req->filename.extension(); is_tpl_extension(ext)) {
                if (try_template(req->filepath))
                  return;
              } else if (const auto tplext = to_tpl_extension(ext); !tplext.empty()) {
                if (try_static_file(req->filepath))
                  return;

                auto fpath = req->filepath;
                fpath.replace_extension(tplext);
                if (try_template(fpath))
                  return;
              } else {
                if (try_static_file(req->filepath))
                  return;
              }
            }

            // Finally, try gener or send error.
            {
              const std::shared_lock lg{self->mutex_};
              if (self->gener_) {
                self->gener_(io, *req);
                return; // done: gener is called
              } /* else ... */
            }
            io->loop_submit([io]
            {
              send_error(io, http::Server_errc::not_found);
            });
            return; // done: error is queued
          } catch (const dmitigr::Exception& e) {
            log::clog()<<"HTTP: "<<e.err().message()<<'\n';
            if (e.condition().category() == http::server_error_category()) {
              io->loop_submit([io, err = e.condition().value()]
              {
                send_error(io, static_cast<http::Server_errc>(err));
              });
              return;
            }
          } catch (...) {
            log::clog()<<"HTTP: unknown error\n";
          }

          io->loop_submit([io]
          {
            send_error(io, http::Server_errc::internal_server_error);
          });
        };

        {
          const std::shared_lock lg{self->mutex_};
          if (self->thread_pool_) {
            self->thread_pool_->submit(handle_request);
            return; // done: request handling is submitted to the thread pool
          } else
            log::clog()<<"HTTP: thread pool is not active, "
              "handling the request on the IO thread!!!\n";
        }
        handle_request();
      };

      // Submit the request handler to the thread pool if available.
      if (method == "POST") {
        DMITIGR_ASSERT(req->body.capacity());
        io->set_receive_handler([io, req, continue_handle_request]
          (const std::string_view data, const bool is_last)
        {
          if (req->body.size() + data.size() > req->body.capacity()) {
            log::clog()<<"HTTP: request body too large\n";
            if (io->is_valid()) {
              send_error(io, http::Server_errc::payload_too_large);
              io->abort();
              DMITIGR_ASSERT(!io->is_valid());
            }
            return;
          } else
            req->body.append(data);

          if (is_last)
            continue_handle_request();
        });
      } else
        continue_handle_request();

      return true; // done: request is scheduled or handled
    } catch (const dmitigr::Exception& e) {
      log::clog()<<"HTTP scheduler: "<<e.err().message()<<'\n';
      if (e.condition().category() == http::server_error_category())
        return send_error(io, static_cast<http::Server_errc>(e.condition().value()));
    } catch (...) {
      log::clog()<<"HTTP scheduler: unknown error\n";
    }
    return send_error(io, http::Server_errc::internal_server_error);
  }

  /// @returns handle(req, io).
  bool operator()(const ws::Http_request& req,
    std::shared_ptr<ws::Http_io> io) const noexcept
  {
    return handle(req, io);
  }

  /// @returns The value of "default language" config option.
  Language default_language() const noexcept
  {
    return default_language_;
  }

  /// @returns The value of "behind proxy" config option.
  bool is_behind_proxy() const noexcept
  {
    return is_behind_proxy_;
  }

private:
  mutable std::shared_mutex mutex_;
  std::filesystem::path docroot_;
  std::vector<std::regex> publics_;
  std::shared_ptr<thread::Pool> thread_pool_;
  Language default_language_{Language::en};
  bool is_behind_proxy_{};
  Auth_checker auth_checker_;
  Auth_prompter auth_prompter_;
  Tpler before_tpler_;
  std::vector<Regex_tpler_pair> tplers_;
  std::vector<Regex_rpcer_pair> rpcers_;
  Tpler after_tpler_;
  Gener gener_;

  inline const static Rpcer invalid_rpcer_;
  inline const static Tpler invalid_tpler_;

  // ---------------------------------------------------------------------------

  /// The constructor.
  explicit Httper(std::shared_ptr<thread::Pool> thread_pool,
    const Config& cfg)
    : thread_pool_{std::move(thread_pool)}
    , default_language_{cfg.default_language()}
    , is_behind_proxy_{cfg.is_behind_proxy()}
  {
    if (to_string_view(default_language_).empty())
      throw Exception{"invalid default language"};
  }
};

} // namespace dmitigr::web

#endif // DMITIGR_WEB_HTTP_HPP
