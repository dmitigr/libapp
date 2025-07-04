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
#include "../../net/net.hpp"

int main()
{
  try {
    namespace net = dmitigr::net;

    const std::string v4_addr_str{"192.168.1.2"};
    DMITIGR_ASSERT(net::Ip_address::is_valid(v4_addr_str));

    auto ip = net::Ip_address::from_text(v4_addr_str);
    DMITIGR_ASSERT(ip);
    DMITIGR_ASSERT(ip.is_valid());
    DMITIGR_ASSERT(ip.family() == net::Protocol_family::ipv4);
    DMITIGR_ASSERT(ip.binary());
    DMITIGR_ASSERT(ip.to_string() == v4_addr_str);

    const std::string invalid_v4_addr_str{"256.168.1.2"};
    DMITIGR_ASSERT(!net::Ip_address::is_valid(invalid_v4_addr_str));

    const std::string v6_addr_str{"fe80::1:2:3:4"};
    ip = net::Ip_address::from_text(v6_addr_str);
    DMITIGR_ASSERT(ip.family() == net::Protocol_family::ipv6);
    DMITIGR_ASSERT(ip.binary());
    DMITIGR_ASSERT(ip.to_string() == v6_addr_str);

    // Comparison.
    {
      net::Ip_address ip1;
      net::Ip_address ip2;
      DMITIGR_ASSERT(!ip1.is_valid());
      DMITIGR_ASSERT(!ip2.is_valid());
      DMITIGR_ASSERT(ip1 == ip2);
    }
    {
      net::Ip_address ip1;
      const auto ip2 = net::Ip_address::from_text("192.168.1.1");
      DMITIGR_ASSERT(!ip1.is_valid());
      DMITIGR_ASSERT(ip2.is_valid());
      DMITIGR_ASSERT(ip1 < ip2);
    }
    {
      const auto ip1 = net::Ip_address::from_text("192.168.1.1");
      const auto ip2 = net::Ip_address::from_text("192.168.1.1");
      DMITIGR_ASSERT(ip1 == ip2);
      const auto ip1_copy = ip1;
      const auto ip2_copy = ip2;
      DMITIGR_ASSERT(ip1 == ip1_copy);
      DMITIGR_ASSERT(ip2 == ip2_copy);
    }
    {
      const auto ip1 = net::Ip_address::from_text("192.168.1.1");
      const auto ip2 = net::Ip_address::from_text("192.168.1.2");
      DMITIGR_ASSERT(ip1 < ip2);
    }
    {
      const auto ip1 = net::Ip_address::from_text("192.168.1.1");
      const auto ip2 = net::Ip_address::from_text("fe80::1:2:3:4");
      DMITIGR_ASSERT(ip1 < ip2);
    }
    {
      const auto ip1 = net::Ip_address::from_text("fe80::1:2:3:4");
      const auto ip2 = net::Ip_address::from_text("fe80::1:2:3:4");
      DMITIGR_ASSERT(ip1 == ip2);
    }

    const int n = 10;
    auto n1 = net::conv(n);
    DMITIGR_ASSERT(n != n1);
    n1 = net::conv(n1);
    DMITIGR_ASSERT(n == n1);

    const float f = 123.456;
    auto f1 = net::conv(f);
    DMITIGR_ASSERT(f != f1);
    f1 = net::conv(f1);
    DMITIGR_ASSERT(f == f1);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
