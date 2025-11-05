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

#ifndef DMITIGR_PGFE_DATA_HPP
#define DMITIGR_PGFE_DATA_HPP

#include "basics.hpp"
#include "dll.hpp"
#include "types_fwd.hpp"

#include <compare>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

namespace dmitigr::pgfe {

/**
 * @ingroup main
 *
 * @brief A data.
 *
 * @details The data in such a representation can be sended to a PostgreSQL
 * server (as the parameter value of the Prepared_statement), or received from
 * the PostgreSQL server (in particular, as the data of the row field or as the
 * asynchronous notification payload).
 *
 * @remarks Bytes of data of format `Data_format::text` are always null-terminated.
 */
class Data {
public:
  /// An alias of Data_format.
  using Format = Data_format;

  /// The destructor.
  virtual ~Data() = default;

  /// @returns `true` if the instance is valid.
  DMITIGR_PGFE_API bool is_valid() const noexcept;

  /// @returns `true` if the instance is valid
  explicit operator bool() const noexcept
  {
    return is_valid();
  }

  /// @name Constructors
  /// @{

  /**
   * @returns A new instance of this class.
   *
   * @remarks bytes() of a resulting Data are null-terminated.
   */
  static DMITIGR_PGFE_API std::unique_ptr<Data> make(
    std::string storage,
    Data_format format = Data_format::text);

  /**
   * @overload
   *
   * @remarks The `bytes` will be copied into the modifiable internal storage.
   * @remarks bytes() of a resulting Data are null-terminated.
   */
  static DMITIGR_PGFE_API std::unique_ptr<Data> make(
    const char* bytes,
    std::size_t size,
    Data_format format);

  /**
   * @overload
   *
   * @par Requires
   * `(storage || !size) && (format != Data_format::text || storage.get()[size] == '\0')`
   */
  static DMITIGR_PGFE_API std::unique_ptr<Data> make(
    std::unique_ptr<void, void(*)(void*)> storage,
    std::size_t size,
    Data_format format);

  /**
   * @returns The deep-copy of this instance.
   *
   * @remarks bytes() of a resulting Data are null-terminated.
   */
  virtual std::unique_ptr<Data> to_data() const = 0;

  /**
   * @returns The result of conversion of text representation of the
   * PostgreSQL's Bytea data type to a plain binary data.
   *
   * @par Requires
   * `(format() == Data_format::text) && (bytes()[size()] == 0)`.
   *
   * @remarks bytes() of a resulting Data are not null-terminated.
   */
  DMITIGR_PGFE_API std::unique_ptr<Data> to_bytea() const;

  /**
   * @brief Similar to Data::to_bytea().
   *
   * @par Requires
   * `text_data`.
   */
  static DMITIGR_PGFE_API std::unique_ptr<Data> to_bytea(const char* text_data);

  /// @}

  /// @name Observers
  /// @{

  /// @returns The data format.
  virtual Data_format format() const noexcept = 0;

  /// @returns The data size in bytes.
  virtual std::size_t size() const noexcept = 0;

  /// @returns `!size()`.
  virtual bool is_empty() const noexcept = 0;

  /// @returns The pointer to a *unmodifiable* memory area.
  virtual const void* bytes() const noexcept = 0;

  /// @}

protected:
  /// @returns `true` if the invariant of this instance is correct.
  virtual bool is_invariant_ok() const;
};

/**
 * @ingroup main
 *
 * @returns
 *   - `std::strong_ordering::less` if the first differing byte in `lhs` is
 *   less than the corresponding byte in `rhs`;
 *   - `std::strong_ordering::equal` if all bytes of `lhs` and `rhs` are equal;
 *   - `std::strong_ordering::greater` if the first differing byte in `lhs` is
 *   greater than the corresponding byte in `rhs`.
 */
DMITIGR_PGFE_API std::strong_ordering
operator<=>(const Data& lhs, const Data& rhs) noexcept;

/**
 * @ingroup main
 *
 * @returns `(lhs <=> rhs) == 0`.
 */
DMITIGR_PGFE_API bool
operator==(const Data& lhs, const Data& rhs) noexcept;

// =============================================================================

/**
 * @ingroup main
 *
 * @brief A data view.
 *
 * @remarks Doesn't owns the data.
 */
class Data_view final : public Data {
public:
  /**
   * @brief Constructs invalid instance.
   *
   * @par Effects
   * `!is_valid()`. `bytes()` returns empty string literal.
   */
  Data_view() = default;

  /**
   * @brief Constructs the data view of the text format.
   *
   * @par Effects
   * `bytes()`. If `bytes` then `is_valid() && (format() == Format::text)`.
   *
   * @remarks bytes() of a resulting Data_view are null-terminated.
   */
  explicit DMITIGR_PGFE_API Data_view(const char* bytes) noexcept;

  /**
   * @brief Constructs the data view of the specified `data`.
   *
   * @par Effects
   * `*this == data`.
   */
  DMITIGR_PGFE_API Data_view(const Data& data) noexcept;

  /// Copy-constructible.
  Data_view(const Data_view&) = default;

  /// Move-constructible.
  DMITIGR_PGFE_API Data_view(Data_view&& rhs) noexcept;

  /// Copy-assignable.
  Data_view& operator=(const Data_view&) = default;

  /// Move-assignable.
  DMITIGR_PGFE_API Data_view& operator=(Data_view&& rhs) noexcept;

  /// Swaps this instance with `rhs`.
  DMITIGR_PGFE_API void swap(Data_view& rhs) noexcept;

  /// @see Data::to_data().
  DMITIGR_PGFE_API std::unique_ptr<Data> to_data() const override;

  /// @see Data::format().
  DMITIGR_PGFE_API Format format() const noexcept override;

  /// @see Data::size().
  DMITIGR_PGFE_API std::size_t size() const noexcept override;

  /// @see Data::is_empty().
  DMITIGR_PGFE_API bool is_empty() const noexcept override;

  /// @see Data::bytes().
  DMITIGR_PGFE_API const void* bytes() const noexcept override;

private:
  friend Copier;
  friend Row;

  Format format_{-1};
  std::string_view data_{"", 0};

  /**
   * @brief Constructs the data view of the specified `size` and `format`.
   *
   * @par Effects
   * `bytes()`. If `bytes` then `is_valid().
   */
  Data_view(const char* bytes, std::size_t size, Format format) noexcept;
};

/**
 * @ingroup main
 *
 * @brief Data_view is swappable.
 */
inline void swap(Data_view& lhs, Data_view& rhs) noexcept
{
  lhs.swap(rhs);
}

} // namespace dmitigr::pgfe

#ifndef DMITIGR_PGFE_NOT_HEADER_ONLY
#include "data.cpp"
#endif

#endif  // DMITIGR_PGFE_DATA_HPP
