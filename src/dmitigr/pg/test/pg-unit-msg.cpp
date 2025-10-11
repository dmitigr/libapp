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

#include "../../base/assert.hpp"
#include "../../pg/msg.hpp"

int main()
{
  try {
    namespace msg = dmitigr::pg::msg;

    // StartupMessage(F)
    {
      msg::Startup_message_view smv1;
      smv1.protocol = dmitigr::host_to_net(std::uint32_t{196610}); // 3.2
      const auto& params = "user\0dmitigr\0database\0dmitigr\0\0";
      smv1.params = {params, std::size(params) - 1};

      std::string message;
      message.resize(serialized_size(smv1));
      serialize(message.data(), smv1);

      const auto smv2 = msg::to_startup_message_view(message.c_str());

      std::cout << smv1 << std::endl;
      std::cout << smv2 << std::endl;

      DMITIGR_ASSERT(smv1 == smv2);
    }

    // Parse(F)
    {
      std::string ps_name{"ps1"};
      std::string query{"select * from table where id = $1"};
      std::vector<std::uint32_t> oids{dmitigr::host_to_net(std::uint32_t{17}),
        dmitigr::host_to_net(std::uint32_t{1983})};

      msg::Parse_view pv1{
        ps_name,
        query,
        dmitigr::host_to_net(static_cast<std::uint16_t>(oids.size())),
        {reinterpret_cast<const char*>(oids.data()), oids.size()*sizeof(oids[0])}};

      std::string message;
      message.resize(serialized_size(pv1));
      serialize(message.data(), pv1);

      const auto pv2 = msg::to_parse_view(message.c_str());

      std::cout << pv1 << std::endl;
      std::cout << pv2 << std::endl;

      DMITIGR_ASSERT(pv1 == pv2);
    }

    // Query(F)
    {
      std::string query{"select 1; select 2; select 3"};
      msg::Query_view qv1{query};

      std::string message;
      message.resize(serialized_size(qv1));
      serialize(message.data(), qv1);

      const auto qv2 = msg::to_query_view(message.c_str());

      std::cout << qv1 << std::endl;
      std::cout << qv2 << std::endl;

      DMITIGR_ASSERT(qv1 == qv2);
    }

    // ReadyForQuery(B)
    {
      msg::Ready_for_query_view rqv1{msg::Tx_status::idle};

      std::string message;
      message.resize(serialized_size(rqv1));
      serialize(message.data(), rqv1);

      const auto rqv2 = msg::to_ready_for_query_view(message.c_str());

      std::cout << rqv1 << std::endl;
      std::cout << rqv2 << std::endl;

      DMITIGR_ASSERT(rqv1 == rqv2);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
