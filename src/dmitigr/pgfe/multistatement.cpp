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

#include "../base/assert.hpp"
#include "conversions.hpp"
#include "exceptions.hpp"
#include "multistatement.hpp"

#include <algorithm>

namespace dmitigr::pgfe {

DMITIGR_PGFE_INLINE Multistatement::Multistatement(const std::string_view input)
{
  Statement::parse([this](auto&& stmt)
  {
    statements_.emplace_back(std::move(stmt));
    return true;
  }, input);
}

DMITIGR_PGFE_INLINE
Multistatement::Multistatement(std::vector<Statement> statements)
  : statements_{std::move(statements)}
{}

DMITIGR_PGFE_INLINE std::size_t Multistatement::size() const noexcept
{
  return statements_.size();
}

DMITIGR_PGFE_INLINE std::size_t Multistatement::empty_query_count() const noexcept
{
  return static_cast<std::size_t>(
    count_if(vector().cbegin(), vector().cend(), [](const auto& stmt)noexcept
    {
      return stmt.is_query_empty();
    }));
}

DMITIGR_PGFE_INLINE bool Multistatement::is_empty() const noexcept
{
  return statements_.empty();
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::string_capacity() const noexcept
{
  std::string::size_type result{};
  if (const auto sz = size()) {
    result += sz - 1; // Space for ';'.
    for (const auto& statement : statements_)
      result += statement.string_capacity();
  }
  return result;
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::query_string_capacity() const
{
  std::string::size_type result{};
  if (const auto sz = size()) {
    result += sz - 1; // Space for ';'.
    for (const auto& statement : statements_)
      result += statement.query_string_capacity();
  }
  return result;
}

DMITIGR_PGFE_INLINE const Statement&
Multistatement::operator[](const std::size_t index) const
{
  if (!(index < size()))
    throw Generic_exception{"cannot get from Multistatement"};
  return statements_[index];
}

DMITIGR_PGFE_INLINE Statement&
Multistatement::operator[](const std::size_t index)
{
  return const_cast<Statement&>(static_cast<const Multistatement&>(*this)[index]);
}

DMITIGR_PGFE_INLINE std::size_t Multistatement::statement_index(
  const std::string_view metadata_key,
  const std::string_view metadata_value,
  const std::size_t offset, const std::size_t metadata_offset) const noexcept
{
  const auto sz = size();
  const auto b = cbegin(statements_);
  const auto e = cend(statements_);
  using Diff = decltype(b)::difference_type;
  const auto i = find_if(std::min(b + static_cast<Diff>(offset),
      b + static_cast<Diff>(sz)), e,
    [&metadata_key, &metadata_value, metadata_offset](const auto& statement)
    {
      const auto& metadata = statement.metadata();
      if (metadata_offset < metadata.size()) {
        const auto index = metadata.index(metadata_key, metadata_offset);
        return (index < metadata.size()) &&
          (metadata.vector()[index].second == metadata_value);
      } else
        return false;
    });
  return static_cast<std::size_t>(i - b);
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::query_absolute_position(const std::size_t index,
  const Connection& conn) const
{
  if (!(index < size()))
    throw Generic_exception{"cannot get query absolute position from Multistatement"};

  const auto junk_size = vector()[index].to_string().size() -
    vector()[index].to_query_string(conn).size();
  const auto statement_position = [this](const std::size_t idx)
  {
    std::string::size_type result{};
    for (std::size_t i{}; i < idx; ++i)
      result += vector()[i].to_string().size() + 1;
    return result;
  };
  return statement_position(index) + junk_size;
}

DMITIGR_PGFE_INLINE void Multistatement::push_back(Statement statement) noexcept
{
  statements_.push_back(std::move(statement));
}

DMITIGR_PGFE_INLINE void Multistatement::insert(const std::size_t index,
  Statement statement)
{
  if (!(index < size()))
    throw Generic_exception{"cannot insert to Multistatement"};
  const auto b = begin(statements_);
  using Diff = decltype(b)::difference_type;
  statements_.insert(b + static_cast<Diff>(index), std::move(statement));
}

DMITIGR_PGFE_INLINE void Multistatement::remove(const std::size_t index)
{
  if (!(index < size()))
    throw Generic_exception{"cannot remove from Multistatement"};
  const auto b = begin(statements_);
  using Diff = decltype(b)::difference_type;
  statements_.erase(b + static_cast<Diff>(index));
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::write_string(char* result) const
{
  const char* const begin{result};
  if (!statements_.empty()) {
    for (const auto& statement : statements_) {
      result += statement.write_string(result);
      *result = ';';
      ++result;
    }
    --result;
  }
  return static_cast<std::string::size_type>(result - begin);
}

DMITIGR_PGFE_INLINE std::string Multistatement::to_string() const
{
  std::string result(string_capacity(), '\0');
  const auto size = write_string(result.data());
  DMITIGR_ASSERT(size <= result.capacity());
  result.resize(size);
  return result;
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::write_query_string(char* result, const Connection* const conn) const
{
  const char* const begin{result};
  if (!statements_.empty()) {
    for (const auto& statement : statements_) {
      result += statement.write_query_string(result, conn);
      *result = ';';
      ++result;
    }
    --result;
  }
  return static_cast<std::string::size_type>(result - begin);
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::write_query_string(char* result, const Connection& conn) const
{
  return write_query_string(result, std::addressof(conn));
}

DMITIGR_PGFE_INLINE std::string::size_type
Multistatement::write_query_string(char* result) const
{
  return write_query_string(result, nullptr);
}

DMITIGR_PGFE_INLINE std::string
Multistatement::to_query_string(const Connection* const conn) const
{
  std::string result(query_string_capacity(), '\0');
  const auto size = write_query_string(result.data(), conn);
  DMITIGR_ASSERT(size <= result.capacity());
  result.resize(size);
  return result;
}

DMITIGR_PGFE_INLINE std::string
Multistatement::to_query_string(const Connection& conn) const
{
  return to_query_string(std::addressof(conn));
}

DMITIGR_PGFE_INLINE std::string Multistatement::to_query_string() const
{
  return to_query_string(nullptr);
}

DMITIGR_PGFE_INLINE const std::vector<Statement>&
Multistatement::vector() const & noexcept
{
  return statements_;
}

DMITIGR_PGFE_INLINE std::vector<Statement>&
Multistatement::vector() & noexcept
{
  return statements_;
}

DMITIGR_PGFE_INLINE std::vector<Statement>
Multistatement::vector() && noexcept
{
  auto result = std::move(statements_);
  statements_ = {};
  return result;
}

} // namespace dmitigr::pgfe
