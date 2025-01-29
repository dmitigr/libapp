// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin

#include "../../lisp/parser.hpp"

#include <iostream>

int main()
{
  const std::string_view es = R"((foreach $x 'pgfe' 'fcgi' #nil (etpl 'ref.thtml')))";
  for (int i{}; i < 10000000; ++i)
    const auto expr = dmitigr::lisp::parse(es);
}
