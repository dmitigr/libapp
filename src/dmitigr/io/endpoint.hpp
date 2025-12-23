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

#include "../base/traits.hpp"

#include <boost/asio.hpp>

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <utility>

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

/// An endpoint.
class Endpoint final {
public:
  /// Constructs an empty endpoint.
  Endpoint() = default;

  /// Constructs an UDS endpoint.
  explicit Endpoint(const std::filesystem::path& path)
    : Endpoint{path.string()}
  {}

  /// @overload
  explicit Endpoint(std::string path)
    : Endpoint{std::move(path), std::string{}}
  {}

  /// Constructs an IP endpoint.
  Endpoint(std::string host, const unsigned int port)
    : Endpoint{std::move(host), std::to_string(port)}
  {}

  /// Generic constructor.
  Endpoint(std::string host, std::string port)
    : host_{std::move(host)}
    , port_{std::move(port)}
  {
    if (host_.empty() && !port_.empty())
      throw std::invalid_argument{"cannot create instance of"
        " dmitigr::io::Endpoint"};
  }

  /// @returns `true` if this instance represents empty endpoint.
  bool is_empty() const noexcept
  {
    return host_.empty();
  }

  /// @returns `true` if this instance represents an UDS endpoint.
  bool is_uds() const noexcept
  {
    return !host_.empty() && port_.empty();
  }

  /// @returns An address or UDS path.
  const std::string& host() const noexcept
  {
    return host_;
  }

  /// @returns A port, or empty string if `is_uds()`.
  const std::string& port() const noexcept
  {
    return port_;
  }

  /// @returns A port if `!is_uds()`, or `0` otherwise.
  int port_number() const
  {
    return !is_uds() ? std::stoi(port_) : 0;
  }

  /// @returns A string representation of this instance.
  std::string to_string() const
  {
    std::string result;
    result.reserve(host_.size() + (!port_.empty() ? 1 + port_.size() : 0));
    result.append(host_);
    if (!port_.empty())
      result.append(":").append(port_);
    return result;
  }

  /**
   * @brief Writes a string representation of this instance to `os`.
   *
   * @returns `os`.
   */
  std::ostream& operator<<(std::ostream& os) const
  {
    if (!host_.empty())
      os<<host_;
    if (!port_.empty())
      os<<':'<<port_;
    return os;
  }

private:
  std::string host_;
  std::string port_;
};

/// @returns `true` if `endpoint` is an IP endpoint.
inline bool
is_ip(const boost::asio::generic::stream_protocol::endpoint& endpoint) noexcept
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

/// @overload
inline std::string to_string(const Endpoint& endpoint)
{
  return endpoint.to_string();
}

/// @returns A specific endpoint of type `E`.
template<class E>
E to_specific_endpoint(const boost::asio::generic::stream_protocol::endpoint& endpoint)
{
  const auto is_family_ok = [&endpoint]() noexcept
  {
    using std::is_same_v;
    if constexpr (is_same_v<E, Tcp_endpoint>) {
      return is_ip(endpoint);
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

inline Endpoint
to_endpoint(const boost::asio::generic::stream_protocol::endpoint& endpoint)
{
  if (is_ip(endpoint)) {
    const auto ep = to_specific_endpoint<Tcp_endpoint>(endpoint);
    return Endpoint{ep.address().to_string(), ep.port()};
  } else if (is_uds(endpoint)) {
    const auto ep = to_specific_endpoint<Uds_endpoint>(endpoint);
    return Endpoint{ep.path()};
  }
  throw std::invalid_argument{"cannot convert "
    "asio::generic::stream_protocol::endpoint to Endpoint"};
}

} // namespace dmitigr::io

#endif  // DMITIGR_IO_ENDPOINT_HPP
