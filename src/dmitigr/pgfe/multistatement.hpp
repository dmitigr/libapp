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

#ifndef DMITIGR_PGFE_MULTISTATEMENT_HPP
#define DMITIGR_PGFE_MULTISTATEMENT_HPP

#include "dll.hpp"
#include "statement.hpp"
#include "types_fwd.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace dmitigr::pgfe {

/**
 * @ingroup utilities
 *
 * @brief A multistatement.
 *
 * @see Statement.
 */
class Multistatement final {
public:
  /// Default-constructible. (Constructs an empty instance.)
  Multistatement() = default;

  /**
   * @brief Constructs the Multistatement from `input`.
   *
   * @details For example, consider the following input:
   *   @code{sql}
   *   -- Comment 1 (comment of the empty statement)
   *   ;
   *
   *   -- Comment 2 (unrelated comment)
   *
   *   -- Comment 3 (related comment)
   *   SELECT 1;
   *
   *   -- Comment 4 (just a footer)
   * @endcode
   * In this case the multistatement will consists of 3 statements:
   *   -# empty statement with only Comment 1;
   *   -# the `SELECT 1` statement with Comment 2 and Comment 3;
   *   -# empty statement with Comment 4.
   *
   * @param input Similar to `input` of Statement::parse().
   *
   * @see Statement::parse().
   */
  explicit DMITIGR_PGFE_API Multistatement(std::string_view input);

  /// @overload
  explicit DMITIGR_PGFE_API Multistatement(std::vector<Statement> statements);

  /// Swaps the instances.
  DMITIGR_PGFE_API void swap(Multistatement& rhs) noexcept;

  /// @returns The count of statements this instance contains.
  DMITIGR_PGFE_API std::size_t size() const noexcept;

  /// @returns The count of statements with empty query this instance contains.
  DMITIGR_PGFE_API std::size_t empty_query_count() const noexcept;

  /// @returns `true` if this instance is empty.
  DMITIGR_PGFE_API bool is_empty() const noexcept;

  /**
   * @returns The statement at `index`.
   *
   * @param index An index of statement to return.
   *
   * @par Requires
   * `index < size()`.
   */
  DMITIGR_PGFE_API const Statement& operator[](std::size_t index) const;

  /// @overload
  DMITIGR_PGFE_API Statement& operator[](std::size_t index);

  /**
   * @returns The index of the statement, or `size()` if no statement that meets
   * the given criterias exists in this multistatement.
   *
   * @param metadata_key A key of the metadata.
   * @param metadata_value A value of the metadata.
   * @param offset A starting position of lookup in this multistatement.
   * @param metadata_offset A starting position of lookup in the metadata.
   *
   * @see Statement::metadata().
   */
  DMITIGR_PGFE_API std::size_t statement_index(
    std::string_view metadata_key,
    std::string_view metadata_value,
    std::size_t offset = 0,
    std::size_t metadata_offset = 0) const noexcept;

  /**
   * @returns The absolute position of the query of the speficied SQL string.
   *
   * @param index An index of SQL string.
   * @param conn A server connection.
   *
   * @par Requires
   * `(index < statement_count()) && conn.is_connected()`.
   */
  DMITIGR_PGFE_API std::string::size_type
  query_absolute_position(std::size_t index, const Connection& conn) const;

  /// Appends the `statement` to this instance.
  DMITIGR_PGFE_API void push_back(Statement statement) noexcept;

  /**
   * @brief Inserts the `statement` to this instance.
   *
   * @par Requires
   * `index < size()`.
   */
  DMITIGR_PGFE_API void insert(std::size_t index, Statement statement);

  /**
   * @brief Removes the statement from this instance.
   *
   * @par Requires
   * `index < size()`.
   */
  DMITIGR_PGFE_API void remove(std::size_t index);

  /// @returns The string of multiple statements.
  DMITIGR_PGFE_API std::string to_string() const;

  /// @returns The query string of multiple statements.
  DMITIGR_PGFE_API std::string to_query_string(const Connection& conn) const;

  /// @returns The underlying vector of statements.
  DMITIGR_PGFE_API const std::vector<Statement>& vector() const & noexcept;

  /// @overload
  DMITIGR_PGFE_API std::vector<Statement>& vector() & noexcept;

  /**
   * @returns The released underlying vector of statements.
   *
   * @par Effects
   * `is_empty()`.
   */
  DMITIGR_PGFE_API std::vector<Statement> vector() && noexcept;

private:
  std::vector<Statement> statements_;
};

/**
 * @ingroup utilities
 *
 * @brief Multistatement is swappable.
 */
inline void swap(Multistatement& lhs, Multistatement& rhs) noexcept
{
  lhs.swap(rhs);
}

} // namespace dmitigr::pgfe

#ifndef DMITIGR_PGFE_NOT_HEADER_ONLY
#include "multistatement.cpp"
#endif

#endif  // DMITIGR_PGFE_MULTISTATEMENT_HPP
