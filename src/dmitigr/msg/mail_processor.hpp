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

#ifndef DMITIGR_MSG_MAIL_PROCESSOR_HPP
#define DMITIGR_MSG_MAIL_PROCESSOR_HPP

#include "../nix/ipc_pipe.hpp"

#include "message.hpp"
#include "processor.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace dmitigr::msg {

class Mail_processor final : public Processor {
public:
  Mail_processor() = default;

  std::unique_ptr<Processor> clone() override
  {
    return std::make_unique<Mail_processor>(*this);
  }

  int process(const Message& message) override
  {
    std::vector<std::string> args{"mail"};
    if (!message.sender.empty()) {
      args.push_back("-r");
      args.push_back(message.sender);
    }
    args.push_back("-s");
    args.push_back(message.subject);
    for (const auto& recipient : message.recipients)
      args.push_back(recipient);
    return nix::ipc::pp::exec_and_wait("mail", args, message.content,
      std::cout, std::cerr);
  }
};

} // namespace dmitigr::msg

#endif  // DMITIGR_MSG_MAIL_PROCESSOR_HPP
