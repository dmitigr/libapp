// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin

#include "../../base/assert.hpp"
#include "../../lisp/parser.hpp"

#include <iostream>

int main()
{
  using namespace dmitigr::lisp;
  using namespace std::literals::string_view_literals;
  using std::dynamic_pointer_cast;

  // ok
  {
    const auto es = ""sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // ok
  {
    const auto es = "#nil"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Nil_expr>(r.res.expr);
    DMITIGR_ASSERT(e);
  }

  // ok
  {
    const auto es = "#true"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<True_expr>(r.res.expr);
    DMITIGR_ASSERT(e);
  }

  // ok
  {
    const auto es = "$x"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Lvar_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->var_name() == "x");
  }

  // ok
  {
    const auto es = "@x"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Gvar_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->var_name() == "x");
  }

  // ok
  {
    const auto es = "1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Integer_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_integer() == 1);
  }

  // ok
  {
    const auto es = "+1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Integer_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_integer() == 1);
  }

  // ok
  {
    const auto es = "-1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Integer_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_integer() == -1);
  }

  // ok
  {
    const auto es = "1."sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Float_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_float() < 1.01 && e->num_float() > 0.99);
  }

  // ok
  {
    const auto es = ".1a"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == 2 && r.res.expr);
  }

  // ok
  {
    const auto es = ".1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Float_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_float() < 0.101 && e->num_float() > 0.099);
  }

  // ok
  {
    const auto es = "+.1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Float_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_float() < 0.101 && e->num_float() > 0.099);
  }

  // ok
  {
    const auto es = "-.1"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Float_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->num_float() < -0.099 && e->num_float() > -0.101);
  }

  // ok
  {
    const auto es = "'The string'"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Str_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->str() == "The string");
  }

  // ok
  {
    const auto es = "()"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
  }

  // ok
  {
    const auto es = " (  foo)"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Tup_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->fun_name() == "foo" && !e->tail_size());
  }

  // ok
  {
    const auto es = "(foo $x)"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
  }

  // ok
  {
    const auto es = "(foo x)"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
  }

  // ok
  {
    const auto es = R"((foreach $x 'pgfe' 'fcgi' #nil
                         (etpl 'ref.thtml')))"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(r && r.res.pos == es.size() && r.res.expr);
    const auto e = dynamic_pointer_cast<Tup_expr>(r.res.expr);
    DMITIGR_ASSERT(e && e->fun_name() == "foreach" && e->tail_size() == 5);
    const auto ea0 = dynamic_pointer_cast<Lvar_expr>(e->tail()[0]);
    DMITIGR_ASSERT(ea0 && ea0->var_name() == "x");
    const auto ea1 = dynamic_pointer_cast<Str_expr>(e->tail()[1]);
    DMITIGR_ASSERT(ea1 && ea1->str() == "pgfe");
    const auto ea2 = dynamic_pointer_cast<Str_expr>(e->tail()[2]);
    DMITIGR_ASSERT(ea2 && ea2->str() == "fcgi");
    const auto ea3 = dynamic_pointer_cast<Nil_expr>(e->tail()[3]);
    DMITIGR_ASSERT(ea3);
    const auto ea4 = dynamic_pointer_cast<Tup_expr>(e->tail()[4]);
    DMITIGR_ASSERT(ea4);
    DMITIGR_ASSERT(ea4->fun_name() == "etpl");
    DMITIGR_ASSERT(ea4->tail_size() == 1);
    const auto ea4a0 = dynamic_pointer_cast<Str_expr>(ea4->tail()[0]);
    DMITIGR_ASSERT(ea4a0);
    DMITIGR_ASSERT(ea4a0->str() == "ref.thtml");
    std::cout << e->to_string() << std::endl;
  }

  // ---------------------------------------------------------------------------

  // err
  {
    const auto es = "."sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // err
  {
    const auto es = "+"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // err
  {
    const auto es = "-"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // err
  {
    const auto es = ".+-"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // err
  {
    const auto es = "("sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }

  // err
  {
    const auto es = "(foo $x"sv;
    const auto r = parse(es);
    DMITIGR_ASSERT(!r && r.res.pos == es.size() && !r.res.expr);
  }
}
