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

#ifndef DMITIGR_JRPC_RESPONSE_HPP
#define DMITIGR_JRPC_RESPONSE_HPP

#include "../base/assert.hpp"
#include "../rajson/conversions.hpp"
#include "basics.hpp"
#include "errctg.hpp"
#include "types_fwd.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace dmitigr::jrpc {

/// A response.
class Response {
public:
  /// The destructor.
  virtual ~Response() = default;

  /// @name Constructors
  /// @{

  /// @returns A new instance of Response.
  static std::unique_ptr<Response> make(std::string_view input);

  /// @}

  /// @returns A String specifying the version of the JSON-RPC protocol.
  virtual std::string_view jsonrpc() const = 0;

  /**
   * @returns A response identifier which the same as the value of the id member
   * in the Request. If there was an error in detecting the id in the Request
   * (e.g. Parse error/Invalid Request), the returned value is `Null`.
   */
  virtual const rapidjson::Value& id() const = 0;

  /// @returns The result of serialization of this instance to a JSON string.
  virtual std::string to_string() const = 0;

  /// @return The allocator.
  virtual rapidjson::Value::AllocatorType& allocator() const = 0;

private:
  friend Error;
  friend Result;

  Response() = default;
};

// =============================================================================

/// An error response.
class Error final : public Response, public Exception {
public:
  /**
   * @brief Constructs an instance with code of Server_errc::server_error,
   * with null ID, and empty message.
   */
  Error()
    : Error{Server_errc::server_error, std::string{}}
  {
    init__(rapidjson::Value{}, std::string{});
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// The constructor.
  Error(const std::error_code code, const Null /*id*/,
    const std::string& message = {})
    : Error{code, message}
  {
    init__(rapidjson::Value{}, message);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @overload
  template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
  Error(const std::error_code code, const T id,
    const std::string& message = {})
    : Error{code, message}
  {
    init__(rapidjson::Value{id}, message);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @overload
  Error(const std::error_code code,
    const std::string_view id, const std::string& message = {})
    : Error{code, message}
  {
    // Attention: calling allocator() assumes constructed rep_!
    init__(rapidjson::Value{id.data(), id.size(), allocator()}, message);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// Swaps this instance with `rhs`.
  void swap(Error& rhs) noexcept
  {
    rep_.swap(rhs.rep_);
  }

  /// @see Response::jsonrpc().
  std::string_view jsonrpc() const override
  {
    return std::string_view{"2.0", 3};
  }

  /// @see Response::id().
  const rapidjson::Value& id() const override
  {
    const auto i = rep_->FindMember("id");
    DMITIGR_ASSERT(i != rep_->MemberEnd());
    return i->value;
  }

  /// @see Response::to_string().
  std::string to_string() const override
  {
    return rajson::to_text(*rep_);
  }

  /**
   * @returns A Primitive or Structured value that contains additional
   * information about the error, or `nullptr` if omitted.
   */
  const rapidjson::Value* data() const
  {
    const auto& e = error();
    const auto i = e.FindMember("data");
    return i != e.MemberEnd() ? &i->value : nullptr;
  }

  /// Sets the additional information about the error.
  void set_data(rapidjson::Value value)
  {
    auto& err = error();
    if (const auto i = err.FindMember("data"); i == err.MemberEnd())
      err.AddMember("data", std::move(value), allocator());
    else
      i->value = std::move(value);
  }

  /// @overload
  template<typename T>
  void set_data(T&& value)
  {
    set_data(rajson::to_value(std::forward<T>(value), allocator()));
  }

  /// @see Response::allocator().
  rapidjson::Value::AllocatorType& allocator() const override
  {
    return rep_->GetAllocator();
  }

private:
  friend Request;
  friend Response;

  std::shared_ptr<rapidjson::Document> rep_;

  bool is_invariant_ok() const
  {
    if (!rep_)
      return false;

    const auto mc = rep_->MemberCount();
    if (mc != 3)
      return false;

    const auto e = rep_->MemberEnd();

    const auto ji = rep_->FindMember("jsonrpc");
    if (ji == e || !(ji->value.IsString() &&
        (rajson::to<std::string_view>(ji->value) == std::string_view{"2.0", 3})))
      return false;

    const auto ii = rep_->FindMember("id");
    if (ii == e || !(ii->value.IsInt() ||
        ii->value.IsString() || ii->value.IsNull()))
      return false;

    const auto ei = rep_->FindMember("error");
    if (ei == e || !ei->value.IsObject())
      return false;

    const auto eimc = ei->value.MemberCount();
    const auto ee = ei->value.MemberEnd();
    const auto ci = ei->value.FindMember("code");
    if (ci == ee || !ci->value.IsInt())
      return false;

    const auto mi = ei->value.FindMember("message");
    if (mi == ee || !mi->value.IsString())
      return false;

    const auto di = ei->value.FindMember("data");
    if (di == ee) {
      if (eimc != 2)
        return false;
    } else if (eimc != 3)
      return false;

    return true;
  }

  const rapidjson::Value& error() const
  {
    const auto i = rep_->FindMember("error");
    DMITIGR_ASSERT(i != rep_->MemberEnd());
    return i->value;
  }

  rapidjson::Value& error()
  {
    return const_cast<rapidjson::Value&>(
      static_cast<const Error*>(this)->error());
  }

  // Used by Request.
  Error(const std::error_code code, rapidjson::Value&& id,
    const std::string& message = {})
    : Error{code, message}
  {
    init__(std::move(id), message);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  // Used by Request.
  Error(const std::error_code code, const rapidjson::Value& id,
    const std::string& message = {})
    : Error{code, message}
  {
    init__(rapidjson::Value{id, allocator()}, message);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  // Used by Response.
  Error(const std::error_code code, const std::string& message,
    std::shared_ptr<rapidjson::Document> rep)
    : Exception{code, message}
    , rep_{std::move(rep)}
  {
    DMITIGR_ASSERT(rep_ != nullptr);
    // Maybe uninitialized, so invariant *maybe* invalid here!
  }

  // Used for pre initialization
  explicit Error(const std::error_code code,
    const std::string& message = {})
    : Error{code, message,
      std::make_shared<rapidjson::Document>(rapidjson::kObjectType)}
  {
    // Uninitialized, so invariant is invalid here!
  }

  void init__(rapidjson::Value&& id, const std::string& message)
  {
    auto& alloc = allocator();
    rep_->AddMember("jsonrpc", "2.0", alloc);
    rep_->AddMember("id", std::move(id), alloc);
    {
      using T = rapidjson::Type;
      using V = rapidjson::Value;
      V e{T::kObjectType};
      e.AddMember("code", V{code().value()}, alloc);
      e.AddMember("message", message, alloc);
      rep_->AddMember("error", std::move(e), alloc);
    }
  }
};

// =============================================================================

/// Represents success of a server method invocation.
class Result final : public Response {
public:
  /// The default constructor.
  Result()
  {
    init__(rapidjson::Value{});
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// The constructor.
  explicit Result(const int id)
  {
    init__(rapidjson::Value{id});
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// @overload
  explicit Result(const std::string_view id)
  {
    // Attention: calling allocator() assumes constructed rep_!
    init__(rapidjson::Value{id.data(), id.size(), allocator()});
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /// Copy-constructible.
  Result(const Result& rhs)
  {
    rep_.CopyFrom(rhs.rep_, allocator(), true);
  }

  /// Copy-assignable.
  Result& operator=(const Result& rhs)
  {
    if (this != &rhs) {
      Result tmp{rhs};
      swap(tmp);
    }
    return *this;
  }

  /// Move-constructible.
  Result(Result&& rhs) = default;

  /// Move-assignable.
  Result& operator=(Result&& rhs) = default;

  /// Swaps this instance with `rhs`.
  void swap(Result& rhs) noexcept
  {
    rep_.Swap(rhs.rep_);
  }

  /// @see Response::jsonrpc()
  std::string_view jsonrpc() const override
  {
    return std::string_view{"2.0", 3};
  }

  /// @see Response::id()
  const rapidjson::Value& id() const override
  {
    const auto i = rep_.FindMember("id");
    DMITIGR_ASSERT(i != rep_.MemberEnd());
    return i->value;
  }

  /// @see Response::to_string()
  std::string to_string() const override
  {
    return rajson::to_text(rep_);
  }

  /// @see Response::allocator()
  rapidjson::Value::AllocatorType& allocator() const override
  {
    return rep_.GetAllocator();
  }

  /// @returns The value determined by the method invoked on the server.
  const rapidjson::Value& data() const
  {
    const auto i = rep_.FindMember("result");
    DMITIGR_ASSERT(i != rep_.MemberEnd());
    return i->value;
  }

  /// Sets the mandatory information about the success.
  void set_data(rapidjson::Value value)
  {
    data__() = std::move(value);
  }

  /// @overload
  template<typename T>
  void set_data(T&& value)
  {
    set_data(rajson::to_value(std::forward<T>(value), allocator()));
  }

private:
  friend Request;
  friend Response;

  mutable rapidjson::Document rep_{rapidjson::Type::kObjectType};

  bool is_invariant_ok() const
  {
    const auto mc = rep_.MemberCount();
    const auto e = rep_.MemberEnd();
    const auto ji = rep_.FindMember("jsonrpc");
    const auto ri = rep_.FindMember("result");
    const auto ii = rep_.FindMember("id");
    return ji != e && ri != e && ii != e &&
      (rajson::to<std::string_view>(ji->value) == std::string_view{"2.0", 3}) &&
      (ii->value.IsInt() || ii->value.IsString() || ii->value.IsNull()) &&
      (mc == 3);
  }

  rapidjson::Value& data__()
  {
    return const_cast<rapidjson::Value&>(
      static_cast<const Result*>(this)->data());
  }

  // Used by Request.
  explicit Result(const rapidjson::Value& id)
  {
    init__(rapidjson::Value{id, allocator()});
    DMITIGR_ASSERT(is_invariant_ok());
  }

  // Used by Response.
  explicit Result(rapidjson::Document&& rep)
    : rep_{std::move(rep)}
  {
    DMITIGR_ASSERT(is_invariant_ok());
  }

  void init__(rapidjson::Value&& id)
  {
    auto& alloc = allocator();
    rep_.AddMember("jsonrpc", "2.0", alloc);
    rep_.AddMember("result", rapidjson::Value{}, alloc);
    rep_.AddMember("id", std::move(id), alloc);
  }
};

// =============================================================================

inline std::unique_ptr<Response> Response::make(const std::string_view input)
{
  auto rep = rajson::document_from_text(input);
  if (!rep.IsObject())
    throw Exception{"JSON-RPC response is not an object"};

  if (rep.MemberCount() != 3)
    throw Exception{"invalid member count in JSON-RPC response"};

  const auto e = rep.MemberEnd();

  // Checking jsonrpc member.
  if (const auto i = rep.FindMember("jsonrpc"); i != e) {
    if (i->value.IsString()) {
      const auto jsonrpc = rajson::to<std::string_view>(i->value);
      constexpr std::string_view ver{"2.0", 3};
      if (jsonrpc != ver)
        throw Exception{"invalid value of \"jsonrpc\" member of "
          "JSON-RPC response"};
    } else
      throw Exception{"invalid type of \"jsonrpc\" member of "
        "JSON-RPC response"};
  } else
    throw Exception{"no \"jsonrpc\" member found in JSON-RPC response"};

  // Checking id member.
  if (const auto i = rep.FindMember("id"); i != e) {
    if (!i->value.IsNumber() && !i->value.IsString() && !i->value.IsNull())
      throw Exception{"invalid type of \"id\" member of JSON-RPC response"};
  } else
    throw Exception{"no \"id\" member found in JSON-RPC response"};

  // Checking result/error member.
  const auto ri = rep.FindMember("result");
  const auto ei = rep.FindMember("error");

  if (ri != e) {
    if (ei != e)
      throw Exception{"both \"result\" and \"error\" member found in "
        "JSON-RPC response"};
    else
      return std::unique_ptr<Result>(new Result{std::move(rep)});
  } else if (ei != e) {
    if (ei->value.IsObject()) {
      std::size_t expected_error_member_count = 3;
      auto& ev = ei->value;
      const auto ee = ev.MemberEnd();

      // Checking error.code member.
      const auto codei = ev.FindMember("code");
      if (codei != ee) {
        if (!codei->value.IsInt())
          throw Exception{"invalid type of \"error.code\" member of "
            "JSON-RPC response"};
      } else
        throw Exception{"no \"error.code\" member found in JSON-RPC response"};

      // Checking error.message member.
      const auto msgi = ev.FindMember("message");
      if (msgi != ee) {
        if (!msgi->value.IsString())
          throw Exception{"invalid type of \"error.message\" member of "
            "JSON-RPC response"};
      } else
        throw Exception{"no \"error.message\" member found in "
          "JSON-RPC response"};

      // Checking error.data member.
      if (const auto i = ev.FindMember("data"); i == ee)
        expected_error_member_count--;

      // Checking member count of error member.
      if (ev.MemberCount() != expected_error_member_count)
        throw Exception{"invalid member count of \"error\" member of "
          "JSON-RPC response"};

      // Done.
      return std::unique_ptr<Error>{new Error{
        std::error_code{rajson::to<int>(codei->value),
          server_error_category()},
        rajson::to<std::string>(msgi->value),
        std::make_shared<rapidjson::Document>(std::move(rep))}};
    } else
      throw Exception{"invalid type of \"error\" member of "
        "JSON-RPC response"};
  } else
    throw Exception{"nor \"result\" nor \"error\" member found in "
      "JSON-RPC response"};
}

} // namespace dmitigr::jrpc

#endif  // DMITIGR_JRPC_RESPONSE_HPP
