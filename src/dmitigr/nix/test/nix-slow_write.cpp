// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

int signalled;

void signal_handler(const int sig)
{
  signalled = sig;
}

int main()
{
  std::signal(SIGTERM, signal_handler);

  for (int i{}; i < 10; ++i) {
    std::cout << i << std::endl;
    if (i > 5)
      std::cerr << i << std::endl;
    if (signalled) {
      std::cerr << "signalled " << signalled << std::endl;
      if (i > 7)
        std::exit(0);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
  }
}
