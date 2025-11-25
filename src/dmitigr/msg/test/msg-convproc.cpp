// -*- C++ -*-
//
// Copyright 2025 Dmitry Igrishin

#include "../../base/assert.hpp"
#include "../../base/str.hpp"
#include "../../msg/msg.hpp"

#include <iostream>

namespace msg = dmitigr::msg;
namespace str = dmitigr::str;

class Cat_processor final : public msg::Processor {
public:
  Cat_processor() = default;

  std::unique_ptr<Processor> clone() override
  {
    return std::make_unique<Cat_processor>(*this);
  }

  int process(const msg::Message& message) override
  {
    std::string content;
    std::vector<std::string> args{"cat"};
    args.push_back("-n");
    if (!message.sender.empty())
      content.append("sender: ").append(message.sender).append("\n");
    content.append("recipients: ").append(str::to_string(message.recipients, ","))
      .append("\n");
    content.append("subject: ").append(message.subject).append("\n");
    content.append("content: ").append(message.content).append("\n");
    return dmitigr::nix::ipc::pp::exec_and_wait("cat", args, content,
      std::cout, std::cerr);
  }
};

int main()
{
  try {
    msg::Sqlite_conveyor conveyor{"msg.db"};
    DMITIGR_ASSERT(!conveyor.is_started());
    conveyor.start();
    DMITIGR_ASSERT(conveyor.is_started());

    conveyor.spool({"sender@example.com",
                    {"recipient1@example.com", "recipient2@example.com"},
        "dmitigr::msg test", "Test content."});
    Cat_processor processor{};
    while (conveyor.process(processor));

    conveyor.stop();
    DMITIGR_ASSERT(!conveyor.is_started());
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
