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

#ifndef DMITIGR_LISP_LIB_HPP
#define DMITIGR_LISP_LIB_HPP

#include "../base/utility.hpp"
#include "../str/stream.hpp"
#include "expr.hpp"

#include <cassert>
#include <filesystem>
#include <mutex>

namespace dmitigr::lisp::lib {

namespace detail {

/**
 * This is a workaround for `dst.insert(cend(dst), cbegin(src), cend(src))`
 * which doesn't works when `dst` and `src` are same containers.
 */
template<typename D, typename S>
inline void push_back(D& dst, const S& src)
{
  const auto dstsz = dst.size(), srcsz = src.size();
  const auto newsz = dstsz + srcsz;
  dst.resize(newsz);
  for (std::size_t i{dstsz}, j{}; i < newsz; ++i, ++j)
    dst[i] = src[j];
}

inline void push_back_recursive(Tuple& dst, const Tuple& src)
{
  for (const auto& e : src) {
    if (is_tup(e))
      push_back_recursive(dst, e->tup());
    else
      dst.push_back(e);
  }
}

inline auto read_to_str(const std::filesystem::path& path)
{
  static const auto read = static_cast<
    std::string(*)(const std::filesystem::path&)>(&str::read_to_string);
  return call_noexcept(read, path);
}

} // namespace detail

// -----------------------------------------------------------------------------
// Binders
// -----------------------------------------------------------------------------

/// Function `let`.
inline Ret_expr fun_let(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "let");
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz == 0)
    return Nil_expr::instance();
  else if (asz > 2)
    return Err(Errc::fun_usage, fun.fun_name());

  if (auto r = args[0]->eval(env)) {
    if (is_tup(r.res)) {
      if (asz == 1)
        return Nil_expr::instance();
      const auto& bindings = r.res->tup();
      const auto bsz = bindings.size();
      if (bsz % 2)
        return Err(Errc::fun_usage, fun.fun_name());
      auto shadowed_env = env;
      for (std::size_t i{}; i < bsz; i += 2) {
        if (is_lvar(bindings[i])) {
          if (auto r = bindings[i + 1]->eval(shadowed_env))
            shadowed_env.set(bindings[i]->var_name(), r.res);
          else
            return r;
        } else
          return Err{Errc::fun_usage, fun.fun_name()};
      }
      return fun.back()->eval(shadowed_env);
    } else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

// -----------------------------------------------------------------------------

/// Special function `set`.
inline Ret_expr fun_set(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "set");
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz % 2)
    return Err(Errc::fun_usage, fun.fun_name());

  auto result = Nil_expr::instance();
  for (std::size_t i{}; i < asz; i += 2) {
    const auto& var = args[i];
    if (is_var(var)) {
      auto r = args[i + 1]->eval(env);
      if (!r)
        return r;
      else if (is_lvar(var))
        env.set(var->var_name(), r.res);
      else if (is_gvar(var)) {
        const std::unique_lock lg{Env::global_mutex};
        Env::global().set(var->var_name(), r.res);
      } else
        DMITIGR_ASSERT(false);
      result = r.res;
    } else
      return Err{Errc::fun_usage, fun.fun_name()};
  }
  return result;
}

// -----------------------------------------------------------------------------

/// Function `copy`.
inline Ret_expr fun_copy(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "copy");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz > 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (asz) {
    if (auto r = (*arg)->eval(env))
      return r.res->clone();
    else
      return r;
  } else
    return Nil_expr::instance();
}

// -----------------------------------------------------------------------------
// Control flow
// -----------------------------------------------------------------------------

/// Special function `if`.
inline Ret_expr fun_if(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "if");
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 2 || asz > 3)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = args[0]->eval(env)) {
    if (is_nil(r.res))
      // ELSE
      return asz > 2 ? args[2]->eval(env) : Nil_expr::instance();
    else
      // THEN
      return args[1]->eval(env);
  } else
    return r;
}

// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_when_unless(const Tup_expr& fun, Env& env, const bool is_when)
{
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1 || asz > 2)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = args[0]->eval(env)) {
    if (is_when ^ is_nil(r.res))
      return asz > 1 ? args[1]->eval(env) : Nil_expr::instance();
    else
      return Nil_expr::instance();
  } else
    return r;
}
} // namespace detail

/// Special function `when`.
inline Ret_expr fun_when(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "when");
  return detail::fun_when_unless(fun, env, true);
}

/// Special function `unless`.
inline Ret_expr fun_unless(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "unless");
  return detail::fun_when_unless(fun, env, false);
}

// -----------------------------------------------------------------------------

namespace detail {
/// Common finisher to both `begin` and `while` (or `until`).
inline Ret_expr fun_finish(const Tup_expr& fun, Env& env, const Errc marker)
{
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz > 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  auto result = Nil_expr::instance();
  if (asz) {
    if (auto r = (*arg)->eval(env))
      result = std::move(r.res);
    else
      return r;
  }
  return Ret_expr{Err{marker}, std::move(result)};
}
} // namespace detail

// -----------------------------------------------------------------------------

/// Special function `begin`.
inline Ret_expr fun_begin(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "begin");
  auto result = Nil_expr::instance();
  for (auto a = fun.tail(), e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env))
      result = std::move(r.res);
    else if (r.err == Errc::unhandled_end)
      return r.res;
    else
      return r;
  }
  return result;
}

/// Function `end`.
inline Ret_expr fun_end(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "end");
  return detail::fun_finish(fun, env, Errc::unhandled_end);
}

// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_while_until(const Tup_expr& fun, Env& env, const bool is_while)
{
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1 || asz > 2)
    return Err{Errc::fun_usage, fun.fun_name()};

  while (true) {
    if (auto r = args[0]->eval(env)) {
      if (is_while ^ is_nil(r.res)) {
        if (asz > 1) {
          if (auto r = args[1]->eval(env); !r) {
            if (r.err == Errc::unhandled_break)
              return r.res;
            else
              return r;
          }
        }
      } else
        return Nil_expr::instance();
    } else
      return r;
  }
}
} // namespace detail

/// Special function `while`.
inline Ret_expr fun_while(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "while");
  return detail::fun_while_until(fun, env, true);
}

/// Special function `until`.
inline Ret_expr fun_until(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "until");
  return detail::fun_while_until(fun, env, false);
}

/// Function `break`.
inline Ret_expr fun_break(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "break");
  return detail::fun_finish(fun, env, Errc::unhandled_break);
}

// -----------------------------------------------------------------------------

/**
 * Special function `catch`.
 *
 * (catch (ERROR-HANDLER-PAIR...) BODY)
 */
inline Ret_expr fun_catch(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "catch");
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 3 || !(asz % 2))
    return Err{Errc::fun_usage, fun.fun_name()};

  // Evaluate BODY. Return immediately if no error.
  auto rbody = fun.back()->eval(env);
  if (!rbody) {
    // Search for a handler associated with an error.
    for (std::size_t i{}; i < asz - 1; i += 2) {
      if (auto r = args[i]->eval(env)) {
        if (is_err(r.res)) {
          if (rbody.err == r.res->err())
            return args[i + 1]->eval(env);
        } else if (is_true(r.res))
          return args[i + 1]->eval(env);
        else
          return Err{Errc::fun_usage, fun.fun_name()};
      } else
        return r;
    }
  }
  return rbody;
}

// -----------------------------------------------------------------------------

/// Special function `throw`.
inline Ret_expr fun_throw(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "throw");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_err(r.res))
      return r.res->err();
    else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r.err;
}

// -----------------------------------------------------------------------------
// Conditionals
// -----------------------------------------------------------------------------

/// Function `and`.
inline Ret_expr fun_and(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "and");
  auto result = True_expr::instance();
  for (auto a = fun.tail(), e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env)) {
      if (is_nil(r.res))
        return Nil_expr::instance();
      else
        result = std::move(r.res);
    } else
      return r;
  }
  return result;
}

/// Function `or`.
inline Ret_expr fun_or(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "or");
  auto result = Nil_expr::instance();
  for (auto a = fun.tail(), e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env)) {
      if (is_nil(r.res))
        result = std::move(r.res);
      else
        return r;
    } else
      return r;
  }
  return result;
}

/// Function `not`.
inline Ret_expr fun_not(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "not");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env))
    return bool_expr(is_nil(r.res));
  else
    return r;
}

// -----------------------------------------------------------------------------
// Math
// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_math_calc(const Tup_expr& fun, Env& env, const int def0,
  Err (Num_expr_base::*op)(const Shared_expr&))
{
  if (auto r = eval(fun.tail(), fun.end(), env)) {
    const auto calc = [&fun, def0, &op, &r](auto&& result) -> Ret_expr
    {
      auto& eargs = r.res;
      auto easz = eargs.size();
      if (!easz)
        return Err{Errc::fun_usage, fun.fun_name()};

      if (easz == 1) {
        eargs.insert(cbegin(eargs), make_expr<Integer_expr>(def0));
        easz = eargs.size();
      }

      if (auto e = result->num_set(eargs[0]))
        return e;

      for (std::size_t i{1}; i < easz; ++i) {
        if (auto e = (result.get()->*op)(eargs[i]))
          return Ret_expr::make_error(Err{e.code(), fun.fun_name()});
      }
      return Ret_expr::make_result(result);
    };

    if (has_float_expr(cbegin(r.res), cend(r.res)))
      return calc(std::make_shared<Float_expr>());
    else
      return calc(std::make_shared<Integer_expr>());
  } else
    return r.err;
}

inline Ret_expr fun_math_calcm(const Tup_expr& fun, Env& env,
  Err (Expr::*op)(const Shared_expr&))
{
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  auto r0 = args[0]->eval(env);
  if (!r0)
    return r0;
  const auto n0 = r0.res;
  if (!is_num(n0))
    return Err{Errc::expr_not_number, fun.fun_name()};

  for (auto a = args + 1, e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env)) {
      if (auto eop = (n0.get()->*op)(r.res))
        return eop;
    } else
      return r;
  }
  return r0;
}

} // namespace detail

/// Function `math-add`.
inline Ret_expr fun_math_add(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-add" || fun.fun_name() == "add");
  return detail::fun_math_calc(fun, env, 0, &Expr::num_add);
}

/// Function `math-add=`.
inline Ret_expr fun_math_addm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-add=" || fun.fun_name() == "add=");
  return detail::fun_math_calcm(fun, env, &Expr::num_add);
}

/// Function `math-sub`.
inline Ret_expr fun_math_sub(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-sub" || fun.fun_name() == "sub");
  return detail::fun_math_calc(fun, env, 0, &Expr::num_sub);
}

/// Function `math-sub=`.
inline Ret_expr fun_math_subm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-sub=" || fun.fun_name() == "sub=");
  return detail::fun_math_calcm(fun, env, &Expr::num_sub);
}

/// Function `math-mul`.
inline Ret_expr fun_math_mul(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-mul" || fun.fun_name() == "mul");
  return detail::fun_math_calc(fun, env, 1, &Expr::num_mul);
}

/// Function `math-mul=`.
inline Ret_expr fun_math_mulm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-mul=" || fun.fun_name() == "mul=");
  return detail::fun_math_calcm(fun, env, &Expr::num_mul);
}

/// Function `math-div`.
inline Ret_expr fun_math_div(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-div" || fun.fun_name() == "div");
  return detail::fun_math_calc(fun, env, 1, &Expr::num_div);
}

/// Function `math-div=`.
inline Ret_expr fun_math_divm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "math-div=" || fun.fun_name() == "div=");
  return detail::fun_math_calcm(fun, env, &Expr::num_div);
}

// -----------------------------------------------------------------------------
// Comparisons
// -----------------------------------------------------------------------------

namespace detail {
template<typename Op>
inline Ret_expr fun_cmp(const Tup_expr& fun, Env& env, const Op& op)
{
  const auto asz = fun.tail_size();
  if (asz < 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  const auto cmp = [&env, &op](auto first, const auto last) -> Ret<bool>
  {
    const auto [err, lhs] = (*first)->eval(env);
    if (err)
      return err;

    for (++first; first != last; ++first) {
      const auto [err, rhs] = (*first)->eval(env);
      if (err)
        return err;
      else if (auto r = lhs->cmp(rhs)) {
        if (!op(r.res))
          return false;
      } else
        return r.err;
    }
    return true;
  };

  for (auto a = fun.tail(), e = fun.end(); a != e; ++a) {
    if (auto rcmp = cmp(a, e)) {
      if (!rcmp.res)
        return Nil_expr::instance();
    } else
      return rcmp.err;
  }
  return True_expr::instance();
}
} // namespace detail

/**
 * Function `lt`.
 *
 * (lt EXPR...)
 */
inline Ret_expr fun_lt(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "lt?");
  return detail::fun_cmp(fun, env, [](const int cmp_res){return cmp_res < 0;});
}

/**
 * Function `le`.
 *
 * (le EXPR...)
 */
inline Ret_expr fun_le(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "le?");
  return detail::fun_cmp(fun, env, [](const int cmp_res){return cmp_res <= 0;});
}

/**
 * Function `eq`.
 *
 * (eq EXPR...)
 */
inline Ret_expr fun_eq(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "eq?");
  return detail::fun_cmp(fun, env, [](const int cmp_res){return cmp_res == 0;});
}

/**
 * Function `ge`.
 *
 * (ge EXPR...)
 */
inline Ret_expr fun_ge(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "ge?");
  return detail::fun_cmp(fun, env, [](const int cmp_res){return cmp_res >= 0;});
}

/**
 * Function `gt`.
 *
 * (gt EXPR...)
 */
inline Ret_expr fun_gt(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "gt?");
  return detail::fun_cmp(fun, env, [](const int cmp_res){return cmp_res > 0;});
}

// -----------------------------------------------------------------------------
// String
// -----------------------------------------------------------------------------

/// Function `string`.
inline Ret_expr fun_string(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "string");
  std::string result;
  for (auto a = fun.tail(), e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env)) {
      if (is_str(r.res))
        result += r.res->str();
      else
        return Err{Errc::fun_usage, fun.fun_name()};
    } else
      return r;
  }
  return make_expr<Str_expr>(std::move(result));
}

/**
 * Function `string-size`.
 *
 * (string-size STR)
 */
inline Ret_expr fun_string_size(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "string-size");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res))
      return make_expr<Integer_expr>(r.res->str().size());
    else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_string_cat(const Tup_expr& fun, Env& env, const bool is_modifier)
{
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  auto r0 = args[0]->eval(env);
  if (!r0)
    return r0;

  auto str = r0.res;
  if (!is_str(str))
    return Err{Errc::fun_usage, fun.fun_name()};
  if (!is_modifier)
    str = str->clone();

  auto& value = str->str();
  for (auto a = args + 1, e = fun.end(); a != e; ++a) {
    if (auto r = (*a)->eval(env)) {
      if (is_str(r.res))
        value += r.res->str();
      else
        return Err{Errc::fun_usage, fun.fun_name()};
    } else
      return r;
  }
  return Ret_expr::make_result(str);
}
} // namespace detail

/**
 * Function `string-cat`.
 *
 * (string-cat STR...)
 */
inline Ret_expr fun_string_cat(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "string-cat" || fun.fun_name() == "cat");
  return detail::fun_string_cat(fun, env, false);
}

/**
 * Modifier `string-cat=`.
 *
 * (string-cat= STR...)
 */
inline Ret_expr fun_string_catm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "string-cat=" || fun.fun_name() == "cat=");
  return detail::fun_string_cat(fun, env, true);
}

// -----------------------------------------------------------------------------
// Tuple
// -----------------------------------------------------------------------------

/// Function `tuple`.
inline Ret_expr fun_tuple(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple");
  if (!fun.tail_size())
    return make_expr<Tup_expr>();

  if (auto r = eval(fun.tail(), fun.end(), env))
    return make_expr<Tup_expr>(std::move(r.res));
  else
    return r.err;
}

/**
 * Function `tuple-size`.
 *
 * (tuple-size TUPLE)
 */
inline Ret_expr fun_tuple_size(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple-size");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_tup(r.res))
      return make_expr<Integer_expr>(r.res->tup().size());
    else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

// -----------------------------------------------------------------------------

/**
 * Function `tuple-flat`.
 *
 * (tuple-flat [ELEMS])
 */
inline Ret_expr fun_tuple_flat(const Tup_expr& fun, Env& env)
{
  auto result = make_expr<Tup_expr>();
  auto& tuple = result->tup();
  for (auto i = fun.tail(), e = fun.end(); i != e; ++i) {
    if (auto r = (*i)->eval(env)) {
      if (is_tup(r.res))
        detail::push_back_recursive(tuple, r.res->tup());
      else
        tuple.push_back(r.res);
    } else
      return r;
  }
  return Ret_expr::make_result(result);
}

// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_tuple_append(const Tup_expr& fun, Env& env, const bool is_modifier)
{
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  auto r0 = args[0]->eval(env);
  if (!r0)
    return r0;

  auto tup = r0.res;
  if (!is_tup(tup))
    return Err{Errc::fun_usage, fun.fun_name()};
  if (!is_modifier)
    tup = tup->clone();

  if (auto r = eval(args + 1, fun.end(), env)) {
    auto& tuple = tup->tup();
    push_back(tuple, r.res);
    return Ret_expr::make_result(tup);
  } else
    return r.err;
}
} // namespace detail

/**
 * Function `tuple-append`.
 *
 * (tuple-append TUP ELEMS...)
 */
inline Ret_expr fun_tuple_append(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple-append");
  return detail::fun_tuple_append(fun, env, false);
}

/**
 * Modifier `tuple-append=`.
 *
 * (tuple-append= TUP ELEMS...)
 */
inline Ret_expr fun_tuple_appendm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple-append=");
  return detail::fun_tuple_append(fun, env, true);
}

// -----------------------------------------------------------------------------

namespace detail {
inline Ret_expr fun_tuple_transform(const Tup_expr& fun, Env& env,
  const bool is_modifier)
{
  assert((!is_modifier && fun.fun_name() == "tuple-transform") ||
    (is_modifier && fun.fun_name() == "tuple-transform="));
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 2)
    return Err{Errc::fun_usage, fun.fun_name()};

  auto r0 = args[0]->eval(env);
  if (!r0)
    return r0;

  if (!is_tup(r0.res))
    return Err{Errc::fun_usage, fun.fun_name()};

  const auto tsz = r0.res->tup().size();
  if (tsz != 2)
    return Err{Errc::fun_usage, fun.fun_name()};

  const auto& var = r0.res->tup()[0];
  if (!is_lvar(var))
    return Err{Errc::fun_usage, fun.fun_name()};

  auto shadowed_env = env;
  if (auto r = r0.res->tup()[1]->eval(shadowed_env)) {
    if (is_tup(r.res)) {
      auto tup = is_modifier ? r.res : r.res->clone();
      for (auto& elem : tup->tup()) {
        shadowed_env.set(var->var_name(), elem);
        if (auto r = args[1]->eval(shadowed_env))
          elem = std::move(r.res);
        else
          return r;
      }
      return tup;
    } else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}
} // namespace detail

/**
 * Function `tuple-transform`.
 *
 * (tuple-transform ($var TUP) BODY)
 */
inline Ret_expr fun_tuple_transform(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple-transform");
  return detail::fun_tuple_transform(fun, env, false);
}

/**
 * Modifier `tuple-transform=`.
 *
 * (tuple-transform= ($var TUP) BODY)
 */
inline Ret_expr fun_tuple_transformm(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "tuple-transform=");
  return detail::fun_tuple_transform(fun, env, true);
}


// -----------------------------------------------------------------------------
// Error
// -----------------------------------------------------------------------------

/// Function `error`.
inline Ret_expr fun_error(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error");
  const auto args = fun.tail();
  const auto asz = fun.tail_size();
  if (asz < 1 || asz > 2)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = eval(args, fun.end(), env)) {
    const auto& eargs = r.res;
    const auto condition = eargs[0];
    if (!is_integer(condition))
      return Err{Errc::fun_usage, fun.fun_name()};

    Shared_expr what; // assigned in the `if` below
    if (asz > 1) {
      if (is_str(eargs[1]))
        what = eargs[1];
      else
        return Err{Errc::fun_usage, fun.fun_name()};
    }

    const std::error_code code{static_cast<int>(condition->num_integer()),
      user_error_category()};
    return make_expr<Err_expr>(what ? Err{code, what->str()} : Err{code});
  } else
    return r.err;
}

// -----------------------------------------------------------------------------

/// Function `error?`.
inline Ret_expr fun_is_error(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error?");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env))
    return bool_expr(is_err(r.res));
  else
    return r;
}

// -----------------------------------------------------------------------------

/// Function `error-code`.
inline Ret_expr fun_error_code(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error-code");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_err(r.res))
      return make_expr<Integer_expr>(r.res->err().code().value());
    else
      return Nil_expr::instance();
  } else
    return r;
}

// -----------------------------------------------------------------------------

/// Function `error-what`.
inline Ret_expr fun_error_what(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error-what");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_err(r.res))
      return make_expr<Str_expr>(r.res->err().what());
    else
      return Nil_expr::instance();
  } else
    return r;
}

// -----------------------------------------------------------------------------

/// Function `error-message`.
inline Ret_expr fun_error_message(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error-message");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_err(r.res))
      return make_expr<Str_expr>(r.res->err().message());
    else
      return Nil_expr::instance();
  } else
    return r;
}

// -----------------------------------------------------------------------------

/// Function `error-category`.
inline Ret_expr fun_error_category(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "error-category");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_err(r.res))
      return make_expr<Str_expr>(r.res->err().code().category().name());
    else
      return Nil_expr::instance();
  } else
    return r;
}

// -----------------------------------------------------------------------------
// filesystem
// -----------------------------------------------------------------------------

/**
 * Function `fs-file-size`.
 *
 * (fs-file-size path)
 */
inline Ret_expr fun_fs_file_size(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "fs-file-size");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res)) {
      namespace fs = std::filesystem;
      std::error_code ec;
      const auto& file_name = r.res->str();
      const auto result = fs::file_size(file_name, ec);
      if (!ec)
        return make_expr<Integer_expr>(result);
      else // throw by default
        return Err{ec, file_name};
    } else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

/**
 * Function `fs-file-data`.
 *
 * (fs-file-data path)
 */
inline Ret_expr fun_fs_file_data(const Tup_expr& fun, Env& env)
{
  assert(fun.fun_name() == "fs-file-data");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res)) {
      const auto& file_name = r.res->str();
      auto [err, data] = detail::read_to_str(file_name);
      if (!err)
        return make_expr<Str_expr>(std::move(data));
      else // throw by default
        return err;
    } else
      return Err{Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

// =============================================================================

/// Initializes the Lisp Standard Library.
void init()
{
  funers() = {
    // binders
    {"let", &fun_let},
    {"set", &fun_set},
    {"copy", &fun_copy},
    // control flow
    {"if", &fun_if},
    {"when", &fun_when},
    {"unless", &fun_unless},
    {"begin", &fun_begin},
    {"end", &fun_end},
    {"while", &fun_while},
    {"until", &fun_until},
    {"break", &fun_break},
    {"catch", &fun_catch},
    {"throw", &fun_throw},
    // conditionals
    {"and", &fun_and},
    {"or", &fun_or},
    {"not", &fun_not},
    // math
    {"math-add", &fun_math_add},
    {"math-add=", &fun_math_addm},
    {"math-sub", &fun_math_sub},
    {"math-sub=", &fun_math_subm},
    {"math-mul", &fun_math_mul},
    {"math-mul=", &fun_math_mulm},
    {"math-div", &fun_math_div},
    {"math-div=", &fun_math_divm},
    // comparisons
    {"lt?", &fun_lt},
    {"le?", &fun_le},
    {"eq?", &fun_eq},
    {"ge?", &fun_ge},
    {"gt?", &fun_gt},
    // string
    {"string", &fun_string},
    {"string-size", &fun_string_size},
    {"string-cat", &fun_string_cat},
    {"string-cat=", &fun_string_catm},
    // tuple
    {"tuple", &fun_tuple},
    {"tuple-size", &fun_tuple_size},
    {"tuple-flat", &fun_tuple_flat},
    {"tuple-append", &fun_tuple_append},
    {"tuple-append=", &fun_tuple_appendm},
    {"tuple-transform", &fun_tuple_transform},
    {"tuple-transform=", &fun_tuple_transformm},
    // error
    {"error", &fun_error},
    {"error?", &fun_is_error},
    {"error-code", &fun_error_code},
    {"error-what", &fun_error_what},
    {"error-message", &fun_error_message},
    {"error-category", &fun_error_category},
    // filesystem
    {"fs-file-size", &fun_fs_file_size},
    {"fs-file-data", &fun_fs_file_data},
    // aliases
    {"cat", &fun_string_cat},
    {"cat=", &fun_string_catm},
    {"add", &fun_math_add},
    {"add=", &fun_math_addm},
    {"sub", &fun_math_sub},
    {"sub=", &fun_math_sub},
    {"mul", &fun_math_mul},
    {"mul=", &fun_math_mulm},
    {"div", &fun_math_div},
    {"div=", &fun_math_divm},
  };
}

} // namespace dmitigr::lisp::lib

#endif  // DMITIGR_LISP_LIB_HPP
