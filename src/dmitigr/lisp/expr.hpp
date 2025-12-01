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

#ifndef DMITIGR_LISP_EXPR_HPP
#define DMITIGR_LISP_EXPR_HPP

#include "../base/assert.hpp"
#include "../base/ret.hpp"
#include "errctg.hpp"
#include "types_fwd.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::lisp {

/// The alais of shared expression.
using Shared_expr = std::shared_ptr<Expr>;
/// The alias of tuple type.
using Tuple = std::vector<Shared_expr>;
/// The alias of Ret<Shared_expr>.
using Ret_expr = Ret<Shared_expr>;
/// The alias of Ret<Tuple>.
using Ret_tuple = Ret<Tuple>;
/// An alias of function handler.
using Funer = std::function<Ret_expr(const Tup_expr&, Env&)>;

/// Function handlers.
static auto& funers() noexcept
{
  static std::vector<std::pair<std::string, Funer>> result;
  return result;
}

/// An environment for passing upon of function call.
class Env final {
private:
  mutable std::vector<std::pair<std::string, Shared_expr>> exprs_;

  auto expr_iter(const std::string_view name) const noexcept
  {
    const auto b = begin(exprs_);
    const auto e = end(exprs_);
    return find_if(b, e, [&name](const auto& p){return p.first == name;});
  }

public:
  /// The global environment mutex.
  static inline std::shared_mutex global_mutex;

  /// The global environment.
  static auto& global() noexcept
  {
    static Env result;
    return result;
  }

  /// Updates the environment.
  Env& set(const std::string_view name, const Shared_expr& expr)
  {
    const auto i = expr_iter(name);
    if (i != cend(exprs_))
      i->second = expr;
    else
      exprs_.emplace_back(std::string{name}, expr);
    return *this;
  }

  /// @returns The expression of environment.
  Ret_expr expr(const std::string_view name) const noexcept
  {
    const auto i = expr_iter(name);
    if (i != cend(exprs_))
      return i->second;
    else
      return Err{Errc::var_unbound, std::string{name}};
  }

  /// @returns `true` if the variable `name` is bound.
  bool is_bound(const std::string_view name) const noexcept
  {
    return expr_iter(name) != cend(exprs_);
  }
};

// -----------------------------------------------------------------------------
// Expr
// -----------------------------------------------------------------------------

/// Local variable type identifier.
constexpr int type_lvar{-1};
/// Global variable type identifier.
constexpr int type_gvar{-2};
/// Function type identifier.
constexpr int type_fun{-3};
/// Error type identifier.
constexpr int type_err{-4};
/// Nil type identifier.
constexpr int type_nil{-5};
/// True type identifier.
constexpr int type_true{-6};
/// Integer type identifier.
constexpr int type_integer{-7};
/// Float type identifier.
constexpr int type_float{-8};
/// String type identifier.
constexpr int type_str{-9};
/// Tuple type identifier.
constexpr int type_tup{-10};

/// An expression.
class Expr : public std::enable_shared_from_this<Expr> {
public:
  /// The destructor.
  virtual ~Expr() = default;

  /// @returns The type of this expression.
  virtual int type() const noexcept = 0;

  // ---------------------------------------------------------------------------

  /// @returns The name of variable.
  virtual const std::string& var_name() const noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns The name of function.
  virtual const std::string& fun_name() const noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// Evaluates a function.
  virtual Ret_expr fun_eval(const Tup_expr&, Env&) const
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns The error.
  virtual const Err& err() const noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns `true` if this expression represents the boolean True.
  virtual bool boolean() const noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns The reference to integer.
  virtual Integer& num_integer() noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// @returns The value of integer.
  virtual Integer num_integer() const noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// @returns The reference to float.
  virtual Float& num_float() noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// @returns The value of float.
  virtual Float num_float() const noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// Sets the value of number.
  virtual Err num_set(const Shared_expr&) noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// Adds the number to number.
  virtual Err num_add(const Shared_expr&) noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// Subtracts the number from number.
  virtual Err num_sub(const Shared_expr&) noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// Multiplies the number by number.
  virtual Err num_mul(const Shared_expr&) noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// Divides the number by number.
  virtual Err num_div(const Shared_expr&) noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns The string.
  virtual std::string& str() noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// @overload
  virtual const std::string& str() const noexcept
  {
    DMITIGR_ASSERT(false);
  }

  /// @returns The tuple.
  virtual Tuple& tup() noexcept
  {
    DMITIGR_ASSERT(false);
  }
  /// @overload
  virtual const Tuple& tup() const noexcept
  {
    DMITIGR_ASSERT(false);
  }
  // ---------------------------------------------------------------------------

  /// @returns A deep-copy of this instance.
  virtual Shared_expr clone() const = 0;

  /// Evaluates this expression.
  virtual Ret_expr eval(Env&) const
  {
    return std::const_pointer_cast<Expr>(shared_from_this());
  }

  /// @returns The string representation of this expression.
  virtual std::string to_string() const = 0;

  /// @returns The output of this expression.
  virtual Ret<std::string> to_output() const
  {
    return to_string();
  }

  /// @returns `-1`, `0`, `1` if this `<`, `==`, `>` than `rhs`.
  virtual Ret<int> cmp(const Shared_expr&) const noexcept
  {
    return Err{Errc::expr_undefined_cmp};
  }
};

/// @return `true` if `e` is a local variable.
inline bool is_lvar(const Shared_expr& e) noexcept
{
  return e->type() == type_lvar;
}

/// @return `true` if `e` is a global variable.
inline bool is_gvar(const Shared_expr& e) noexcept
{
  return e->type() == type_gvar;
}

/// @return `true` if `e` is a variable.
inline bool is_var(const Shared_expr& e) noexcept
{
  return is_lvar(e) || is_gvar(e);
}

/// @return `true` if `e` is a function.
inline bool is_fun(const Shared_expr& e) noexcept
{
  return e->type() == type_fun;
}

/// @return `true` if `e` is a error.
inline bool is_err(const Shared_expr& e) noexcept
{
  return e->type() == type_err;
}

/// @return `true` if `e` is Nil.
inline bool is_nil(const Shared_expr& e) noexcept
{
  return e->type() == type_nil;
}

/// @return `true` if `e` is True.
inline bool is_true(const Shared_expr& e) noexcept
{
  return e->type() == type_true;
}

/// @return `true` if `e` is a boolean.
inline bool is_bool(const Shared_expr& e) noexcept
{
  return is_nil(e) || is_true(e);
}

/// @return `true` if `e` is an integer.
inline bool is_integer(const Shared_expr& e) noexcept
{
  return e->type() == type_integer;
}

/// @return `true` if `e` is a float.
inline bool is_float(const Shared_expr& e) noexcept
{
  return e->type() == type_float;
}

/// @return `true` if `e` is a number.
inline bool is_num(const Shared_expr& e) noexcept
{
  return is_integer(e) || is_float(e);
}

/// @return `true` if `e` is a string.
inline bool is_str(const Shared_expr& e) noexcept
{
  return e->type() == type_str;
}

/// @return `true` if `e` is a tuple.
inline bool is_tup(const Shared_expr& e) noexcept
{
  return e->type() == type_tup;
}

/// @returns The new instance.
template<class E, typename ... Types>
inline Shared_expr make_expr(Types&& ... args)
{
  static_assert(std::is_base_of_v<Expr, E>);
  return std::make_shared<E>(std::forward<Types>(args)...);
}

/// @return The tuple of evaluated expressions.
template<typename In>
inline Ret_tuple eval(In first, In last, Env& env)
{
  Ret_tuple result;
  for (; first != last; ++first) {
    if (auto r = (*first)->eval(env))
      result.res.push_back(std::move(r.res));
    else
      return Ret_tuple{r.err};
  }
  return result;
}

/// @returns `true` if there is a Float expression in range `[first, last)`.
template<typename In>
inline bool has_float_expr(const In first, const In last) noexcept
{
  return find_if(first, last, is_float) != last;
}

// -----------------------------------------------------------------------------
// Var
// -----------------------------------------------------------------------------

/// A variable expression.
class Var_expr : public Expr {
public:
  explicit Var_expr(std::string name)
    : name_{std::move(name)}
  {}

  const std::string& var_name() const noexcept override
  {
    return name_;
  }

private:
  std::string name_;
};

/// A local variable expression.
class Lvar_expr final : public Var_expr {
public:
  using Var_expr::Var_expr;

  int type() const noexcept override
  {
    return type_lvar;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Lvar_expr>(*this);
  }

  Ret_expr eval(Env& env) const override
  {
    return env.expr(var_name());
  }

  std::string to_string() const override
  {
    return std::string{"$"}.append(var_name());
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_lvar(rhs))
      return var_name() < rhs->var_name() ? -1 : var_name() == rhs->var_name() ? 0 : 1;
    else
      return Err{Errc::expr_not_lvar};
  }

private:
  mutable Shared_expr val_;
};

/// A global variable expression.
class Gvar_expr final : public Var_expr {
public:
  using Var_expr::Var_expr;

  int type() const noexcept override
  {
    return type_gvar;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Gvar_expr>(*this);
  }

  Ret_expr eval(Env&) const override
  {
    const std::shared_lock sl{Env::global_mutex};
    return Env::global().expr(var_name());
  }

  std::string to_string() const override
  {
    return std::string{"@"}.append(var_name());
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_gvar(rhs))
      return var_name() < rhs->var_name() ? -1 : var_name() == rhs->var_name() ? 0 : 1;
    else
      return Err{Errc::expr_not_gvar};
  }
private:
  mutable Shared_expr val_;
};

// -----------------------------------------------------------------------------
// Fun
// -----------------------------------------------------------------------------

/// A function expression.
class Fun_expr final : public Expr {
public:
  /// Constructs the new instance.
  explicit Fun_expr(std::string name) noexcept
    : name_{std::move(name)}
  {}

  int type() const noexcept override
  {
    return type_fun;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Fun_expr>(*this);
  }

  const std::string& fun_name() const noexcept override
  {
    return name_;
  }

  Ret_expr fun_eval(const Tup_expr& fun, Env& env) const override
  {
    if (!funer_) {
      const auto b = cbegin(funers());
      const auto e = cend(funers());
      const auto f = find_if(b, e, [this](const auto& p)
      {
        return p.first == name_;
      });
      if (f != e)
        funer_ = f->second;
      else
        return Err{Errc::fun_unknown, name_};
    }
    return funer_(fun, env);
  }

  std::string to_string() const override
  {
    return name_;
  }

  Ret<std::string> to_output() const override
  {
    return std::string{"function "}.append(name_);
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_fun(rhs))
      return fun_name() < rhs->fun_name() ? -1 : fun_name() == rhs->fun_name() ? 0 : 1;
    else
      return Err{Errc::expr_not_function};
  }

private:
  std::string name_;
  mutable Funer funer_; // cache
};

// -----------------------------------------------------------------------------
// Err
// -----------------------------------------------------------------------------

/// An error.
class Err_expr final : public Expr {
public:
  explicit Err_expr(Err err)
    : err_{std::move(err)}
  {}

  const Err& err() const noexcept override
  {
    return err_;
  }

  int type() const noexcept override
  {
    return type_err;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Err_expr>(*this);
  }

  std::string to_string() const override
  {
    return std::string{"("}.append("error").append(" ")
      .append(std::to_string(err_.code().value())).append(" ")
      .append("'").append(err_.what()).append("'").append(")");
  }

  Ret<std::string> to_output() const override
  {
    return std::string{"error: "}.append(err_.message());
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_err(rhs))
      return err().code() < rhs->err().code() ? -1
        : err().code() == rhs->err().code() ? 0 : 1;
    else
      return Err{Errc::expr_not_error};
  }

private:
  Err err_;
};

// -----------------------------------------------------------------------------
// Bool
// -----------------------------------------------------------------------------

/// A boolean expression.
class Bool_expr : public Expr {
public:
  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_bool(rhs))
      return boolean() < rhs->boolean() ? -1 : boolean() == rhs->boolean() ? 0 : 1;
    else
      return Err{Errc::expr_not_boolean};
  }
};

/// The special constant `#nil`.
class Nil_expr final : public Bool_expr {
public:
  static Shared_expr instance()
  {
    static const auto result = std::make_shared<Nil_expr>();
    return result;
  }

  int type() const noexcept override
  {
    return type_nil;
  }

  Shared_expr clone() const override
  {
    return std::const_pointer_cast<Expr>(shared_from_this());
  }

  std::string to_string() const override
  {
    return std::string{"#nil"};
  }

  bool boolean() const noexcept override
  {
    return false;
  }
};

/// The special constant `#true`.
class True_expr final : public Bool_expr {
public:
  static Shared_expr instance()
  {
    static const auto result = std::make_shared<True_expr>();
    return result;
  }

  int type() const noexcept override
  {
    return type_true;
  }

  Shared_expr clone() const override
  {
    return std::const_pointer_cast<Expr>(shared_from_this());
  }

  std::string to_string() const override
  {
    return std::string{"#true"};
  }

  bool boolean() const noexcept override
  {
    return true;
  }
};

/// @returns The boolean expression.
template<typename T>
inline Shared_expr bool_expr(const T& b)
{
  return static_cast<bool>(b) ? True_expr::instance() : Nil_expr::instance();
}

// -----------------------------------------------------------------------------
// Num
// -----------------------------------------------------------------------------

#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

/// A number expression base.
class Num_expr_base : public Expr {};

template<typename T>
class Num_expr final : public Num_expr_base {
public:
  Num_expr() = default;

  explicit Num_expr(const T value)
    : value_{value}
  {}

  Integer& num_integer() noexcept override
  {
    if constexpr (std::is_same_v<T, Integer>) {
      return value_;
    } else {
      DMITIGR_ASSERT(false);
    }
  }
  Integer num_integer() const noexcept override
  {
    if constexpr (std::is_same_v<T, Integer>) {
      return value_;
    } else {
      DMITIGR_ASSERT(false);
    }
  }
  Float& num_float() noexcept override
  {
    if constexpr (std::is_same_v<T, Float>) {
      return value_;
    } else {
      DMITIGR_ASSERT(false);
    }
  }
  Float num_float() const noexcept override
  {
    if constexpr (std::is_same_v<T, Float>) {
      return value_;
    } else {
      DMITIGR_ASSERT(false);
    }
  }

  int type() const noexcept override
  {
    if constexpr (std::is_same_v<T, Integer>) {
      return type_integer;
    } else {
      return type_float;
    }
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Num_expr>(*this);
  }

  std::string to_string() const override
  {
    if constexpr (std::is_same_v<T, Integer>) {
      return to_chars(24, value_);
    } else {
      return to_chars(32, value_);
    }
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    static const auto compare = [](const auto lh, const auto rh)noexcept
    {
      return lh < rh ? -1 : lh == rh ? 0 : 1;
    };

    if (is_integer(rhs))
      return compare(value_, rhs->num_integer());
    else if (is_float(rhs))
      return compare(value_, rhs->num_float());
    else
      return Err{Errc::expr_not_number};
  }

  Err num_set(const Shared_expr& rhs) noexcept override
  {
    return set_num_value(value_, rhs, [](auto& lh, const auto& rh)noexcept
    {
      lh = rh;
      return Err{};
    });
  }

  Err num_add(const Shared_expr& rhs) noexcept override
  {
    return set_num_value(value_, rhs, [](auto& lh, const auto& rh)noexcept
    {
      lh += rh;
      return Err{};
    });
  }

  Err num_sub(const Shared_expr& rhs) noexcept override
  {
    return set_num_value(value_, rhs, [](auto& lh, const auto& rh)noexcept
    {
      lh -= rh;
      return Err{};
    });
  }

  Err num_mul(const Shared_expr& rhs) noexcept override
  {
    return set_num_value(value_, rhs, [](auto& lh, const auto& rh)noexcept
    {
      lh *= rh;
      return Err{};
    });
  }

  Err num_div(const Shared_expr& rhs) noexcept override
  {
    return set_num_value(value_, rhs, [](auto& lh, const auto& rh)noexcept
    {
      if (rh) {
        lh /= rh;
        return Err{};
      } else
        return Err{Errc::num_division_by_zero};
    });
  }

private:
  T value_{};

  static std::string to_chars(const std::size_t sz, const T val)
  {
    // FIXME: Visual Studio 2019 16.4+ also fully supports to_chars() for floats.
#if (defined(__GNUG__) && (__GNUC__ >= 11))
    std::string result(sz, '0');
    const auto r = std::to_chars(result.data(), result.data() + result.size(), val);
    if (r.ec == std::errc{})
      result.resize(r.ptr - result.data());
    return result;
#else
    (void)sz;
    return std::to_string(val);
#endif
  }
};

/// Applies `op` to both numeric values: `tgt` and `src`.
template<typename T, typename BinaryOp>
Err set_num_value(T& tgt, const Shared_expr& src, const BinaryOp& op) noexcept
{
  if (is_integer(src))
    return op(tgt, src->num_integer());
  else if (is_float(src))
    return op(tgt, src->num_float());
  else
    return Err{Errc::expr_not_number};
}

#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif

// -----------------------------------------------------------------------------
// Str
// -----------------------------------------------------------------------------

/// A string expression.
class Str_expr final : public Expr {
public:
  explicit Str_expr(std::string value)
    : value_{std::move(value)}
  {}

  const std::string& str() const noexcept override
  {
    return value_;
  }

  std::string& str() noexcept override
  {
    return value_;
  }

  std::string to_string() const noexcept override
  {
    return std::string{"'"}.append(value_).append("'");
  }

  Ret<std::string> to_output() const override
  {
    return value_;
  }

  int type() const noexcept override
  {
    return type_str;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Str_expr>(*this);
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    if (is_str(rhs))
      return str() < rhs->str() ? -1 : str() == rhs->str() ? 0 : 1;
    else
      return Err{Errc::expr_not_string};
  }

private:
  std::string value_;
};

// -----------------------------------------------------------------------------
// Tup
// -----------------------------------------------------------------------------

/// A tuple expression.
class Tup_expr final : public Expr {
public:
  Tup_expr() = default;

  explicit Tup_expr(Tuple value)
    : value_{std::move(value)}
  {}

  const Shared_expr& front() const noexcept
  {
    return value_.front();
  }

  const Shared_expr& back() const noexcept
  {
    return value_.back();
  }

  auto head() const noexcept
  {
    return value_.begin();
  }

  auto end() const noexcept
  {
    return value_.end();
  }

  auto tail_size() const noexcept
  {
    return value_.empty() ? 0 : value_.size() - 1;
  }

  auto tail() const noexcept
  {
    return end() - tail_size();
  }

  const std::string& fun_name() const noexcept override
  {
    DMITIGR_ASSERT(!value_.empty() && is_fun(value_[0]));
    return value_[0]->fun_name();
  }

  const Tuple& tup() const noexcept override
  {
    return value_;
  }

  Tuple& tup() noexcept override
  {
    return value_;
  }

  int type() const noexcept override
  {
    return type_tup;
  }

  Shared_expr clone() const override
  {
    return std::make_shared<Tup_expr>(*this);
  }

  Ret_expr eval(Env& env) const override
  {
    if (!value_.empty() && is_fun(value_[0]))
      return value_[0]->fun_eval(*this, env);
    else
      return Expr::eval(env);
  }

  std::string to_string() const override
  {
    std::string result{"("};
    for (const auto& e : value_) {
      DMITIGR_ASSERT(e);
      result += e->to_string();
      result += ' ';
    }
    if (result.size() > 1)
      result.pop_back();
    result += ")";
    return result;
  }

  Ret<std::string> to_output() const override
  {
    return to_output(0);
  }

  Ret<int> cmp(const Shared_expr& rhs) const noexcept override
  {
    static const auto cmp = [](const Tuple& lh, const Tuple& rh)noexcept -> Ret<int>
    {
      const auto min_sz = std::min(lh.size(), rh.size());
      for (std::size_t i{}; i < min_sz; ++i) {
        if (auto r = lh[i]->cmp(rh[i]); !r || r.res)
          return r;
      }
      return lh.size() < rh.size() ? -1 : lh.size() == rh.size() ? 0 : 1;
    };

    if (is_tup(rhs))
      return cmp(value_, rhs->tup());
    else
      return Err{Errc::expr_not_tuple};
  }

private:
  Tuple value_;

  Ret<std::string> to_output(const std::size_t indent_size) const
  {
    std::string result;
    for (const auto& e : value_) {
      result.append(indent_size, ' ');
      if (is_tup(e)) {
        const auto t = std::static_pointer_cast<Tup_expr>(e);
        if (auto r = t->to_output(indent_size + 2))
          result += r.res;
        else
          return r;
      } else if (const auto r = e->to_output())
        result += r.res;
      else
        return r;
      result += '\n';
    }
    if (!result.empty())
      result.pop_back();
    return result;
  }
};

} // namespace dmitigr::lisp

#endif  // DMITIGR_LISP_EXPR_HPP
