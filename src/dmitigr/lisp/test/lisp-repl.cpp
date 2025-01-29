// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin

#include "../../lisp/lib.hpp"
#include "../../lisp/parser.hpp"

#include <iostream>

int main()
{
  namespace lisp = dmitigr::lisp;
  lisp::lib::init();
  std::cout << "Welcome to Igrilisp! Use CTRL-D to exit.";
  while (std::cin) {
    std::string input;
    bool enter{};
    char ch{};
    std::cout << "\nigrilisp> ";
    while (std::cin.get(ch)) {
      if (ch == '\n') {
        if (enter)
          break;
        else
          enter = true;
      } else {
        enter = false;
        input += ch;
      }
    }
    // std::cout << input << std::endl;
    if (input.empty()) {
      std::cerr << "Empty input" << std::endl;
      continue;
    }

    if (const auto r = lisp::parse(input)) {
      if (r.res.pos != input.size()) {
        std::cerr << "Invalid input" << std::endl;
        continue;
      }
      lisp::Env env;
      const auto [err, exp] = r.res.expr->eval(env);
      if (!err)
        std::cout << exp->to_string() << std::endl;
      else
        std::cerr << "Eval error: " << err.message() << std::endl;
    } else
      std::cerr << "Parse error: " << r.err.message() << std::endl;
  }
}
