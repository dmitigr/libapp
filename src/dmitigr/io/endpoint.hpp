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

#ifndef DMITIGR_IO_ENDPOINT_HPP
#define DMITIGR_IO_ENDPOINT_HPP

#include <boost/asio.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>

namespace dmitigr::io {
namespace detail {

template<class T>
std::string to_string(const T& endpoint)
{
  std::ostringstream os;
  os << endpoint;
  return os.str();
}

} // namespace detail

/// A TCP endpoint.
using Tcp_endpoint = boost::asio::ip::tcp::endpoint;

/// An UDS endpoint.
using Uds_endpoint = boost::asio::local::stream_protocol::endpoint;

/// An unresolved endpoint.
struct Unresolved_endpoint final {
  /// A TCP address or UDS path.
  std::string address;

  /// A TCP port, or zero if `address` stores an UDS path.
  int port{};
};

/// @returns `true` if `endpoint` is a TCP endpoint.
inline bool
is_tcp(const boost::asio::generic::stream_protocol::endpoint& endpoint) noexcept
{
  const auto family = endpoint.protocol().family();
  return family == AF_INET || family == AF_INET6;
}

/// @returns `true` if `endpoint` is an UDS endpoint.
inline bool
is_uds(const boost::asio::generic::stream_protocol::endpoint& endpoint) noexcept
{
  return endpoint.protocol().family() == AF_UNIX;
}

/// @returns The string representation of `endpoint`.
template<class P>
std::string to_string(const boost::asio::ip::basic_endpoint<P>& endpoint)
{
  return detail::to_string(endpoint);
}

/// @overload
template<class P>
std::string to_string(const boost::asio::local::basic_endpoint<P>& endpoint)
{
  return detail::to_string(endpoint);
}

/// @returns A specific endpoint of type `E`.
template<class E>
E to_specific_endpoint(const boost::asio::generic::stream_protocol::endpoint& endpoint)
{
  const auto is_family_ok = [&endpoint]() noexcept
  {
    using std::is_same_v;
    if constexpr (is_same_v<E, Tcp_endpoint>) {
      return is_tcp(endpoint);
    } else if constexpr (is_same_v<E, Uds_endpoint>) {
      return is_uds(endpoint);
    } else
      static_assert(false_value<E>);
  }();
  if (is_family_ok) {
    E result;
    result.resize(endpoint.size());
    std::memcpy(result.data(), endpoint.data(), endpoint.size());
    return result;
  }
  throw std::invalid_argument{"cannot convert generic endpoint to specific endpoint"};
}

inline Unresolved_endpoint
to_unresolved_endpoint(const boost::asio::generic::stream_protocol::endpoint& endpoint)
{
  if (is_tcp(endpoint)) {
    const auto ep = to_specific_endpoint<Tcp_endpoint>(endpoint);
    return Unresolved_endpoint{ep.address().to_string(), ep.port()};
  } else if (is_uds(endpoint)) {
    const auto ep = to_specific_endpoint<Uds_endpoint>(endpoint);
    return Unresolved_endpoint{ep.path()};
  }
  throw std::invalid_argument{"cannot convert generic endpoint to Unresolved_endpoint"};
}

} // namespace dmitigr::io

#endif  // DMITIGR_IO_ENDPOINT_HPP
