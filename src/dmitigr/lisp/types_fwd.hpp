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

#ifndef DMITIGR_LISP_TYPES_FWD_HPP
#define DMITIGR_LISP_TYPES_FWD_HPP

/// The API.
namespace dmitigr::lisp {

/// The alias of integer type.
using Integer = long long;
/// The alias of floating point type.
using Float = long double;

enum class Errc;
class Generic_error_category;

class Env;

class Expr;
class Bool_expr;
class Nil_expr;
class True_expr;
class Err_expr;
class Num_expr_base;
template<typename> class Num_expr;
using Integer_expr = Num_expr<Integer>;
using Float_expr = Num_expr<Float>;
class Var_expr;
class Lvar_expr;
class Gvar_expr;
class Str_expr;
class Fun_expr;
class Tup_expr;

/// The implementation details.
namespace detail {
} // namespace detail

} // namespace dmitigr::lisp

#endif  // DMITIGR_LISP_TYPES_FWD_HPP
