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
#include "../base/str.hpp"
#include "basics.hpp"
#include "errctg.hpp"
#include "exceptions.hpp"
#include "problem.hpp"

#include <cctype>
#include <cerrno>
#include <cstdlib>

namespace dmitigr::pgfe {

DMITIGR_PGFE_INLINE Problem::Problem(detail::pq::Result&& result) noexcept
  : pq_result_{std::move(result)}
{
  DMITIGR_ASSERT(is_invariant_ok());
}

DMITIGR_PGFE_INLINE bool Problem::is_invariant_ok() const noexcept
{
  return !pq_result_ || is_sqlstate(condition());
}

DMITIGR_PGFE_INLINE std::error_code Problem::condition() const noexcept
{
  return {sqlstate_string_to_int(code()), sqlstate_error_category()};
}

DMITIGR_PGFE_INLINE const char* Problem::code() const noexcept
{
  return pq_result_ ? str::coalesce({pq_result_.er_code(), "00000"}) : nullptr;
}

DMITIGR_PGFE_INLINE const char* Problem::brief() const noexcept
{
  return pq_result_.er_brief();
}

DMITIGR_PGFE_INLINE const char* Problem::detail() const noexcept
{
  return pq_result_.er_detail();
}

DMITIGR_PGFE_INLINE const char* Problem::hint() const noexcept
{
  return pq_result_.er_hint();
}

DMITIGR_PGFE_INLINE const char* Problem::query_position() const noexcept
{
  return pq_result_.er_query_position();
}

DMITIGR_PGFE_INLINE std::string::size_type
Problem::query_position_num() const noexcept
{
  const char* const pos{query_position()};
  return pos ? std::strtoul(pos, nullptr, 10) : 0;
}

DMITIGR_PGFE_INLINE const char* Problem::internal_query_position() const noexcept
{
  return pq_result_.er_internal_query_position();
}

DMITIGR_PGFE_INLINE std::string::size_type
Problem::internal_query_position_num() const noexcept
{
  const char* const pos{internal_query_position()};
  return pos ? std::strtoul(pos, nullptr, 10) : 0;
}

DMITIGR_PGFE_INLINE const char* Problem::internal_query() const noexcept
{
  return pq_result_.er_internal_query();
}

DMITIGR_PGFE_INLINE const char* Problem::context() const noexcept
{
  return pq_result_.er_context();
}

DMITIGR_PGFE_INLINE const char* Problem::schema_name() const noexcept
{
  return pq_result_.er_schema_name();
}

DMITIGR_PGFE_INLINE const char* Problem::table_name() const noexcept
{
  return pq_result_.er_table_name();
}

DMITIGR_PGFE_INLINE const char* Problem::column_name() const noexcept
{
  return pq_result_.er_column_name();
}

DMITIGR_PGFE_INLINE const char* Problem::data_type_name() const noexcept
{
  return pq_result_.er_data_type_name();
}

DMITIGR_PGFE_INLINE const char* Problem::constraint_name() const noexcept
{
  return pq_result_.er_constraint_name();
}

DMITIGR_PGFE_INLINE const char* Problem::source_file() const noexcept
{
  return pq_result_.er_source_file();
}

DMITIGR_PGFE_INLINE const char* Problem::source_line() const noexcept
{
  return pq_result_.er_source_line();
}

DMITIGR_PGFE_INLINE std::string::size_type Problem::source_line_num() const noexcept
{
  const char* const pos{source_line()};
  return pos ? std::strtoul(pos, nullptr, 10) : 0;
}

DMITIGR_PGFE_INLINE const char* Problem::source_function() const noexcept
{
  return pq_result_.er_source_function();
}

DMITIGR_PGFE_INLINE std::optional<Problem_severity> Problem::severity() const noexcept
{
  const char* const s{pq_result_.er_severity_non_localized()};
  return s ? to_problem_severity(std::string_view{s}) : std::nullopt;
}

DMITIGR_PGFE_INLINE std::error_code Problem::min_condition() noexcept
{
  return {0, sqlstate_error_category()};
}

DMITIGR_PGFE_INLINE std::error_code Problem::max_condition() noexcept
{
  return {60466175, sqlstate_error_category()};
}

DMITIGR_PGFE_INLINE std::error_code Problem::min_error_condition() noexcept
{
  return {139968, sqlstate_error_category()};
}

DMITIGR_PGFE_INLINE int Problem::sqlstate_string_to_int(const char* const sqlstate)
{
  if (!sqlstate)
    return -1;

  const std::string_view state{sqlstate};
  if (!((state.size() == 5) &&
      std::isalnum(static_cast<unsigned char>(state[0])) &&
      std::isalnum(static_cast<unsigned char>(state[1])) &&
      std::isalnum(static_cast<unsigned char>(state[2])) &&
      std::isalnum(static_cast<unsigned char>(state[3])) &&
      std::isalnum(static_cast<unsigned char>(state[4]))))
    throw Generic_exception{"cannot convert SQLSTATE to int"};

  errno = 0;
  const long int result{std::strtol(state.data(), nullptr, 36)};
  DMITIGR_ASSERT(errno == 0);
  DMITIGR_ASSERT(is_sqlstate(result));
  return result;
}

DMITIGR_PGFE_INLINE std::string Problem::sqlstate_int_to_string(const int value)
{
  if (!is_sqlstate(value))
    throw Generic_exception{"cannot convert int to SQLSTATE"};
  return str::to_string(value, 36);
}

DMITIGR_PGFE_INLINE bool Problem::is_sqlstate(const std::error_code value) noexcept
{
  return value.category() == sqlstate_error_category() && is_sqlstate(value.value());
}

DMITIGR_PGFE_INLINE bool Problem::is_sqlstate(const int value) noexcept
{
  return min_condition().value() <= value && value <= max_condition().value();
}

} // namespace dmitigr::pgfe
