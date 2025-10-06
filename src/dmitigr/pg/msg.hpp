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

#ifndef DMITIGR_PG_MSG_HPP
#define DMITIGR_PG_MSG_HPP

#include "../base/endianness.hpp"
#include "../str/sequence.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string_view>

namespace dmitigr::pg::msg {

/// A message type.
enum class Type : char {
  parse = 'P',
  query = 'Q'
};

/// A message data offset.
constexpr const std::size_t data_offset{1 + 4};

/// @returns The type of `message`.
inline Type type(const char* const message) noexcept
{
  assert(message);
  return static_cast<Type>(message[0]);
}

/// @returns The data part of `message`.
inline const char* data(const char* const message) noexcept
{
  assert(message);
  return message + data_offset;
}

/// @returns The size of serialized `message`.
inline std::uint32_t serialized_size(const char* const message) noexcept
{
  assert(message);
  std::uint32_t result;
  std::memcpy(&result, message + 1, sizeof(result));
  return net_to_host(result);
}

// -----------------------------------------------------------------------------
// Parse(F) message
// -----------------------------------------------------------------------------

/**
 * @brief A Parse message view.
 *
 * @warning Numbers in network byte order.
 */
struct Parse_view final {
  std::string_view ps_name;
  std::string_view query;
  std::uint16_t param_type_count{};
  std::string_view param_type_oids;

  std::uint32_t param_type_oid(const std::uint16_t idx) const noexcept
  {
    std::uint32_t result;
    std::memcpy(&result, param_type_oids.data() + idx*sizeof(result), sizeof(result));
    return result;
  }
};

/// @returns `true` if `lhs` equals to `rhs`.
inline bool operator==(const Parse_view& lhs, const Parse_view& rhs) noexcept
{
  if (lhs.param_type_count != rhs.param_type_count)
    return false;
  else if (const auto ptc = net_to_host(lhs.param_type_count)) {
    for (std::uint16_t i{}; i < ptc; ++i)
      if (lhs.param_type_oid(i) != rhs.param_type_oid(i))
        return false;
  }
  return lhs.ps_name == rhs.ps_name && lhs.query == rhs.query;
}

/// @returns `true` if `lhs` differs from `rhs`.
inline bool operator!=(const Parse_view& lhs, const Parse_view& rhs) noexcept
{
  return !(lhs == rhs);
}

/// @returns `true` if `pv` is valid.
inline bool is_valid(const Parse_view& pv) noexcept
{
  return pv.ps_name.data() && pv.query.data();
}

/// @returns The size of serialized Parse message.
inline std::uint32_t serialized_size(const Parse_view& pv) noexcept
{
  if (!is_valid(pv))
    return 0;

  return data_offset +
    pv.ps_name.size() + 1 + pv.query.size() + 1 +
    sizeof(pv.param_type_count) + pv.param_type_oids.size();
}

/// @returns An instance of Parse_view from `message`.
inline Parse_view to_parse_view(const char* const message) noexcept
{
  if (!message || type(message) != Type::parse)
    return Parse_view{};

  Parse_view result;
  result.ps_name = data(message);
  result.query = {result.ps_name.data() + result.ps_name.size() + 1};

  const auto ptc_offset = result.query.data() + result.query.size() + 1;
  std::memcpy(&result.param_type_count, ptc_offset, sizeof(result.param_type_count));
  if (result.param_type_count)
    result.param_type_oids = {ptc_offset + 2,
      net_to_host(result.param_type_count)*sizeof(std::uint32_t)};
  return result;
}

/**
 * @brief Serializes `pv` into `message`.
 *
 * @par Requires
 * `message` must point to a memory space of size at least serialized_size(pv).
 */
inline void serialize(char* const message, const Parse_view& pv) noexcept
{
  if (!message || !is_valid(pv))
    return;

  message[0] = static_cast<char>(Type::parse);
  const std::uint32_t message_size{host_to_net(serialized_size(pv))};
  std::memcpy(message + 1, &message_size, sizeof(message_size));

  auto* const ps_name = message + data_offset;
  std::memcpy(ps_name, pv.ps_name.data(), pv.ps_name.size());
  ps_name[pv.ps_name.size()] = 0;

  auto* const query = ps_name + pv.ps_name.size() + 1;
  std::memcpy(query, pv.query.data(), pv.query.size());
  query[pv.query.size()] = 0;

  auto* const ptc = query + pv.query.size() + 1;
  std::memcpy(ptc, &pv.param_type_count, sizeof(pv.param_type_count));

  auto* const pto = ptc + 2;
  std::memcpy(pto, pv.param_type_oids.data(), pv.param_type_oids.size());
}

/// Prints `pv` into `os`.
inline std::ostream& operator<<(std::ostream& os, const Parse_view& pv)
{
  if (is_valid(pv)) {
    os << static_cast<char>(Type::parse)
       << '{'
       << serialized_size(pv)
       << ','
       << '"' << pv.ps_name << '"'
       << ','
       << '"' << pv.query << '"'
       << ','
       << '{';
    if (const auto ptc = net_to_host(pv.param_type_count)) {
      for (std::uint16_t i{}; i < ptc - 1; ++i)
        os << net_to_host(pv.param_type_oid(i)) << ',';
      os << net_to_host(pv.param_type_oid(ptc - 1));
    }
    os << '}';
    os << '}';
  }
  return os;
}

// -----------------------------------------------------------------------------
// Query(F) message
// -----------------------------------------------------------------------------

/**
 * @brief A Query message view.
 *
 * @warning Numbers in network byte order.
 */
struct Query_view final {
  std::string_view query;
};

/// @returns `true` if `lhs` equals to `rhs`.
inline bool operator==(const Query_view& lhs, const Query_view& rhs) noexcept
{
  return lhs.query == rhs.query;
}

/// @returns `true` if `lhs` differs from `rhs`.
inline bool operator!=(const Query_view& lhs, const Query_view& rhs) noexcept
{
  return !(lhs == rhs);
}

/// @returns `true` if `qv` is valid.
inline bool is_valid(const Query_view& qv) noexcept
{
  return qv.query.data();
}

/// @returns The size of serialized Query message.
inline std::uint32_t serialized_size(const Query_view& qv) noexcept
{
  return is_valid(qv) ? data_offset + qv.query.size() + 1 : 0;
}

/// @returns An instance of Query_view from `message`.
inline Query_view to_query_view(const char* const message) noexcept
{
  if (!message || type(message) != Type::query)
    return Query_view{};

  Query_view result;
  result.query = data(message);
  return result;
}

/**
 * @brief Serializes `qv` into `message`.
 *
 * @par Requires
 * `message` must point to a memory space of size at least serialized_size(qv).
 */
inline void serialize(char* const message, const Query_view& qv) noexcept
{
  if (!message || !is_valid(qv))
    return;

  message[0] = static_cast<char>(Type::query);
  const std::uint32_t message_size{host_to_net(serialized_size(qv))};
  std::memcpy(message + 1, &message_size, sizeof(message_size));

  auto* const query = message + data_offset;
  std::memcpy(query, qv.query.data(), qv.query.size());
  query[qv.query.size()] = 0;
}

/// Prints `qv` into `os`.
inline std::ostream& operator<<(std::ostream& os, const Query_view& qv)
{
  if (is_valid(qv)) {
    os << static_cast<char>(Type::query)
       << '{'
       << serialized_size(qv)
       << ','
       << '"' << qv.query << '"'
       << '}';
  }
  return os;
}

} // namespace dmitigr::pg::msg

#endif  // DMITIGR_PG_MSG_HPP
