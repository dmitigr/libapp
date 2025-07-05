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

#ifndef DMITIGR_LISP_PARSER_HPP
#define DMITIGR_LISP_PARSER_HPP

#include "expr.hpp"

#include <cctype>
#include <memory>
#include <string>
#include <string_view>

namespace dmitigr::lisp {

/// A parse result.
struct Parse_result final {
  /// Position followed the parsed `expr`.
  std::string_view::size_type pos{};
  /// The parsed expression.
  Shared_expr expr{};
};

/// @returns The error or parse result.
Ret<Parse_result> parse(const std::string_view input)
{
  using Ret = Ret<Parse_result>;

  if (input.empty())
    return Ret::make_error(Errc::parse_empty);

  enum {
    space,
    spec, // #nil, #true, ...
    var,  // $local, @global
    fun,  // function-name
    str,  // 'string'
    num,  // number
    expr  // (elem...)
  } state = space;

  static const auto is_spec_char = [](const char ch) noexcept
  {
    return std::isalnum(ch) || ch == '-';
  };
  static const auto is_var_char = [](const char ch) noexcept
  {
    return std::isalnum(ch) || ch == '-' || ch == '_';
  };
  static const auto is_fun_1st_char = [](const char ch) noexcept
  {
    return std::isalpha(ch);
  };
  static const auto is_fun_char = [](const char ch) noexcept
  {
    return std::isalnum(ch) || ch == '=' || ch == '?' || ch == '-';
  };
  static const auto is_num_char = [](const char ch) noexcept
  {
    return std::isdigit(ch) || ch == '.' || ch == '+' || ch == '-';
  };
  static const auto parse_num = [](const std::string& data, const auto pos)
  {
    char* end{};
    {
      errno = 0;
      const auto val = std::strtoll(data.c_str(), &end, 10);
      if (const auto err = errno)
        return Ret::make_error(Errc::parse_num_invalid, pos);
      else if (data.c_str() + data.size() == end)
        return Ret::make_result(pos, make_expr<Integer_expr>(val));
    }
    {
      errno = 0;
      const auto val = std::strtold(data.c_str(), &end);
      if (const auto err = errno; err || data.c_str() + data.size() != end)
        return Ret::make_error(Errc::parse_num_invalid, pos);
      else
        return Ret::make_result(pos, make_expr<Float_expr>(val));
    }
  };
  static const auto parse_fun = [](std::string&& data, const auto pos)
  {
    return Ret::make_result(pos, make_expr<Fun_expr>(std::move(data)));
  };
  static const auto parse_spec = [](const std::string& data, const auto pos)
  {
    if (data == "nil")
      return Ret::make_result(pos, Nil_expr::instance());
    else if (data == "true")
      return Ret::make_result(pos, True_expr::instance());
    else
      return Ret::make_error(Errc::parse_spec_invalid_name, pos);
  };
  static const auto parse_var = [](std::string&& data,
    const std::string_view::size_type pos, const char prefix)
  {
    if (!data.empty()) {
      if (prefix == '$')
        return Ret::make_result(pos, make_expr<Lvar_expr>(std::move(data)));
      else if (prefix == '@')
        return Ret::make_result(pos, make_expr<Gvar_expr>(std::move(data)));
      else
        DMITIGR_ASSERT(false);
    } else
      return Ret::make_error(Errc::parse_var_invalid_name, pos);
  };

  char prefix{};
  std::string data;
  Tuple tup;
  std::string_view::size_type pos{};
  const std::string_view::size_type sz{input.size()};
  while (pos < sz) {
    const auto ch = input[pos];
    switch (state) {
    case space:
      if (!std::isspace(ch)) {
        switch (ch) {
        case '#':  state = spec; break;
        case '$':  state = var, prefix = ch; break;
        case '@':  state = var, prefix = ch; break;
        case '\'': state = str;   break;
        case '(':  state = expr;  break;
        default:
          if (is_num_char(ch)) {
            state = num;
            goto case_num;
          } else if (is_fun_1st_char(ch)) {
            state = fun;
            goto case_fun;
          } else
            return Ret::make_error(Errc::parse_malformed, pos + 1);
        }
      }
      break;

    case spec:
      if (is_spec_char(ch)) {
        data += ch;
        break;
      } else
        return parse_spec(data, pos);

    case var:
      if (is_var_char(ch)) {
        data += ch;
        break;
      } else
        return parse_var(std::move(data), pos, prefix);

    case str:
      if (ch != '\'') {
        data += ch;
        break;
      } else
        return Ret::make_result(pos + 1, make_expr<Str_expr>(std::move(data)));

    case num:
      if (is_num_char(ch) || ch == 'e') {
      case_num:
        data += ch;
        break;
      } else
        return parse_num(data, pos);

    case fun:
      if (is_fun_char(ch)) {
      case_fun:
        data += ch;
        break;
      } else
        return parse_fun(std::move(data), pos);

    case expr:
      if (ch == ')')
        return Ret::make_result(pos + 1, make_expr<Tup_expr>(std::move(tup)));
      else if (!std::isspace(ch)) {
        if (auto r = parse(input.substr(pos))) {
          tup.push_back(std::move(r.res.expr));
          pos += r.res.pos;
          continue;
        } else
          return Ret::make_error(std::move(r.err),
            pos + r.res.pos, std::move(r.res.expr));
      } else
        break;
    }
    ++pos;
  }
  switch (state) {
  case space: return Ret::make_result(pos);
  case spec: return parse_spec(data, pos);
  case var: return parse_var(std::move(data), pos, prefix);
  case num: return parse_num(data, pos);
  case fun: return parse_fun(std::move(data), pos);
  default: return Ret::make_error(Errc::parse_incomplete, pos);
  }
}

} // namespace dmitigr::lisp

#endif  // DMITIGR_LISP_PARSER_HPP
