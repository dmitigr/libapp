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

#ifndef DMITIGR_WEB_LISP_HPP
#define DMITIGR_WEB_LISP_HPP

#include "../lisp/lisp.hpp"
#include "../str/stream.hpp"
#include "../tpl/generic.hpp"
#include "errc.hpp"
#include "util.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dmitigr::web {

/// Text template type identifier.
constexpr int type_tpl{1};
/// Text template stack type identifier.
constexpr int type_tplstack{2};
/// Httper type identifier.
constexpr int type_httper{3};

inline bool is_tpl(const lisp::Shared_expr& e) noexcept
{
  return e->type() == type_tpl;
}

inline bool is_tplstack(const lisp::Shared_expr& e) noexcept
{
  return e->type() == type_tplstack;
}

inline bool is_httper(const lisp::Shared_expr& e) noexcept
{
  return e->type() == type_httper;
}

class Tpl_expr : public lisp::Expr {
public:
  explicit Tpl_expr(tpl::Generic tpl)
    : tpl_{std::move(tpl)}
  {}

  int type() const noexcept override
  {
    return type_tpl;
  }

  lisp::Shared_expr clone() const override
  {
    return std::make_shared<Tpl_expr>(*this);
  }

  std::string to_string() const override
  {
    return tpl_.to_string("<{{", "}}>");
  }

  Ret<std::string> to_output() const override
  {
    return tpl_.to_output();
  }

  Ret<int> cmp(const lisp::Shared_expr& rhs) const noexcept override
  {
    if (is_tpl(rhs)) {
      const auto rhs_tpl = std::static_pointer_cast<Tpl_expr>(rhs);
      return tpl() < rhs_tpl->tpl() ? -1 : tpl() == rhs_tpl->tpl() ? 0 : 1;
    } else
      return Err{Errc::lisp_expr_not_tpl};
  }

  const tpl::Generic& tpl() const noexcept
  {
    return tpl_;
  }

private:
  tpl::Generic tpl_;
};

class Tplstack_expr : public lisp::Expr {
public:
  Tplstack_expr() = default;

  explicit Tplstack_expr(std::vector<std::filesystem::path> stack)
    : stack_{std::move(stack)}
  {}

  int type() const noexcept override
  {
    return type_tplstack;
  }

  lisp::Shared_expr clone() const override
  {
    return std::make_shared<Tplstack_expr>(*this);
  }

  std::string to_string() const override
  {
    std::string result{"(tplstack"};
    for (const auto& path : stack_)
      result.append(" ").append(path.generic_string());
    result.append(")");
    return result;
  }

  Ret<int> cmp(const lisp::Shared_expr& rhs) const noexcept override
  {
    if (is_tpl(rhs)) {
      const auto rhs_tplstack = std::static_pointer_cast<Tplstack_expr>(rhs);
      return stack() < rhs_tplstack->stack() ? -1 :
        stack() == rhs_tplstack->stack() ? 0 : 1;
    } else
      return Err{Errc::lisp_expr_not_tplstack};
  }

  const std::vector<std::filesystem::path>& stack() const noexcept
  {
    return stack_;
  }

  std::vector<std::filesystem::path>& stack() noexcept
  {
    return stack_;
  }

private:
  std::vector<std::filesystem::path> stack_;
};

// =============================================================================

namespace detail {

inline const auto& str(const lisp::Env& env, const std::string_view name)
{
  const auto ret = env.expr(name);
  DMITIGR_ASSERT(ret);
  DMITIGR_ASSERT(is_str(ret.res));
  return ret.res->str();
}

inline auto rebased(std::filesystem::path path,
  const std::filesystem::path& docroot)
{
  auto rpath = relative(path, docroot);
  DMITIGR_ASSERT(!rpath.empty());
  return path = "$_docroot"/std::move(rpath);
}

inline auto stack_graph(std::vector<std::filesystem::path>& stack,
  const std::filesystem::path& docroot)
{
  for (auto& path : stack)
    path = rebased(path, docroot);
  auto graph = str::to_string(stack, " -> ", [](const auto& e)
  {
    return e.string();
  });
  return graph;
}

inline Err tpl_check_cycle(std::vector<std::filesystem::path>& stack,
  const std::filesystem::path& tplfile,
  const std::filesystem::path& docroot)
{
  const auto beg = cbegin(stack);
  const auto end = cend(stack);
  if (find(beg, end, tplfile) != end) {
    auto graph = stack_graph(stack, docroot);
    graph.append(" -> ").append(tplfile.string());
    return Err{Errc::tpl_cycle, "template reference cyclicity: " + graph};
  }
  return Err{};
}

inline Ret<tpl::Generic>
tpl(const std::filesystem::path& tplfile, lisp::Env& env)
{
  namespace fs = std::filesystem;

  // Get the environment.
  const auto tplstack_ret = env.expr("_tplstack");
  DMITIGR_ASSERT(tplstack_ret);
  DMITIGR_ASSERT(is_tplstack(tplstack_ret.res));
  auto& stack =
    std::static_pointer_cast<Tplstack_expr>(tplstack_ret.res)->stack();
  const fs::path docroot{str(env, "_docroot")};

  // Shadow and modify the environment.
  auto shadowed_env = env;
  shadowed_env.set("_tplorig",
    lisp::make_expr<lisp::Str_expr>(tplfile.generic_string()));

  // Check the possible template reference cyclicity.
  if (auto e = detail::tpl_check_cycle(stack, tplfile, docroot))
    return e;

  // Cope the stack.
  struct Stack_tracker final {
    ~Stack_tracker()
    {
      stack_.pop_back();
    }
    Stack_tracker(decltype(stack)& stack, const fs::path& tplfile)
      : stack_{stack}
    {
      stack_.push_back(tplfile);
    }
    decltype(stack)& stack_;
  };
  const Stack_tracker guard{stack, tplfile};

  // Check the template file existence.
  if (!is_regular_file(tplfile))
    return Err{Errc::file_not_found, stack_graph(stack, docroot)};

  // Read the template into memory.
  const auto input = str::read_to_string(tplfile, true, str::Trim::all);
  auto [err, result] = tpl::Generic::make(input, "<{{", "}}>");
  if (err)
    return err;

  // Evaluate the Lisp expressions from the template parameters.
  for (std::size_t p{}, pcount{result.parameter_count()}; p < pcount;) {
    // Get the parameter name.
    const std::string& pname = result.parameter(p)->name();

    // Try to parse Lisp expression.
    namespace lisp = dmitigr::lisp;
    const auto [parse_err, parse_res] = lisp::parse(pname);
    if (parse_err) {
      ++p;
      continue;
    }

    // Evaluate the Lisp expression.
    const auto [eval_err, eval_res] = parse_res.expr->eval(shadowed_env);
    if (eval_err)
      return eval_err;

    // Replace or bind the parameter with the evaluation result.
    if (eval_res->type() == type_tpl) {
      const auto tpl_expr = std::static_pointer_cast<Tpl_expr>(eval_res);
      if (auto e = result.replace(p, tpl_expr->tpl())) // FIXME: std::move
        return e;
      pcount = result.parameter_count();
    } else if (auto r = eval_res->to_output()){
      result.bind(p, r.res);
      ++p;
    } else
      return r.err;
  }
  return result;
}

inline std::filesystem::path tplfile(const std::filesystem::path& tplfile,
  const lisp::Env& env)
{
  namespace fs = std::filesystem;
  // tplorig - tplfile from what `web-tpl`, `web-esc`, etc functions was called.
  const fs::path tplorig{detail::str(env, "_tplorig")};
  fs::path root{detail::str(env, "_docroot")};
  if (!tplfile.is_absolute())
    root = tplorig.parent_path();
  return root / tplfile.relative_path();
}

} // namespace detail

/// Function `web-raw`.
inline lisp::Ret_expr fun_web_raw(const lisp::Tup_expr& fun, lisp::Env& env)
{
  assert(fun.fun_name() == "web-raw");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{lisp::Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res)) {
      namespace fs = std::filesystem;
      const auto tplfile = detail::tplfile(r.res->str(), env);
      auto [err, res] = str::read_to_string_nothrow(tplfile, true, str::Trim::all);
      if (err)
        return err;
      else if (std::any_of(cbegin(res), cend(res), str::is_zero))
        return Err{Errc::txt_invalid, fun.fun_name()};
      else
        return lisp::make_expr<lisp::Str_expr>(std::move(res));
    } else
      return Err{lisp::Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

/// Function `web-esc`.
inline lisp::Ret_expr fun_web_esc(const lisp::Tup_expr& fun, lisp::Env& env)
{
  assert(fun.fun_name() == "web-esc");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{lisp::Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res)) {
      namespace fs = std::filesystem;
      const auto tplfile = detail::tplfile(r.res->str(), env);
      auto [err, res] = str::read_to_string_nothrow(tplfile, true, str::Trim::all);
      if (err)
        return err;
      else if (std::any_of(cbegin(res), cend(res), str::is_zero))
        return Err{Errc::txt_invalid, fun.fun_name()};
      else
        return lisp::make_expr<lisp::Str_expr>(esc(res));
    } else
      return Err{lisp::Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

/// Function `web-tpl`.
inline lisp::Ret_expr fun_web_tpl(const lisp::Tup_expr& fun, lisp::Env& env)
{
  assert(fun.fun_name() == "web-tpl");
  const auto arg = fun.tail();
  const auto asz = fun.tail_size();
  if (asz != 1)
    return Err{lisp::Errc::fun_usage, fun.fun_name()};

  if (auto r = (*arg)->eval(env)) {
    if (is_str(r.res)) {
      namespace fs = std::filesystem;
      const auto tplfile = detail::tplfile(r.res->str(), env);
      if (auto [err, res] = detail::tpl(tplfile, env); err)
        return err;
      else
        return lisp::make_expr<Tpl_expr>(std::move(res));
    } else
      return Err{lisp::Errc::fun_usage, fun.fun_name()};
  } else
    return r;
}

// =============================================================================

void init_lisp()
{
  lisp::lib::init();
  auto& funers = lisp::funers();
  funers.emplace_back("web-raw", fun_web_raw);
  funers.emplace_back("web-esc", fun_web_esc);
  funers.emplace_back("web-tpl", fun_web_tpl);
}

} // namespace dmitigr::web

#endif  // DMITIGR_WEB_LISP_HPP
