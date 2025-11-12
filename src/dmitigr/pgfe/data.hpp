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
#include <memory>
#include <string>
#include <string_view>
#include <utility>

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

  /// @returns The deep-copy of this instance.
  virtual std::unique_ptr<Data> to_data() const = 0;

  /// @returns The data format.
  virtual Data_format format() const noexcept = 0;

  /// @returns The data size in bytes.
  virtual std::size_t size() const noexcept = 0;

  /// @returns `!size()`.
  virtual bool is_empty() const noexcept = 0;

  /// @returns The pointer to a **unmodifiable** memory area.
  virtual const void* bytes() const noexcept = 0;
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
// Container_data
// =============================================================================

/// The base implementation of Data based on containers.
template<class Container>
class Container_data : public Data {
public:
  using Storage = Container;

  template<class S>
  Container_data(S&& storage, const Format format)
    : format_(format)
    , storage_(std::forward<S>(storage))
  {}

  std::unique_ptr<Data> to_data() const override
  {
    return std::make_unique<Container_data>(storage_, format_);
  }

  Format format() const noexcept override
  {
    return format_;
  }

  std::size_t size() const noexcept override
  {
    return storage_.size();
  }

  bool is_empty() const noexcept override
  {
    return storage_.empty();
  }

  const void* bytes() const noexcept override
  {
    return storage_.data() ? storage_.data() : "";
  }

  Storage& storage() noexcept
  {
    return storage_;
  }

  const Storage& storage() const noexcept
  {
    return storage_;
  }

private:
  Format format_{Format::text};
  Storage storage_;
};

/// The alias of Container_data<std::string>.
using String_data = Container_data<std::string>;

inline std::unique_ptr<Data> make_string_data(std::string storage,
  const Data_format format = Data_format::text)
{
  return std::make_unique<String_data>(std::move(storage), format);
}

// =============================================================================
// Memory_data
// =============================================================================

/// The generic implementation of Data based on a heap storage.
template<typename T, class Deleter = std::default_delete<T>>
class Memory_data : public Data {
public:
  using Storage = std::unique_ptr<T, Deleter>;

  Memory_data(Storage&& storage, const std::size_t size, const Format format)
    : format_(format)
    , size_(size)
    , storage_(std::move(storage))
  {}

  std::unique_ptr<Data> to_data() const override
  {
    return make_string_data(
      std::string(static_cast<const char*>(bytes()), size()), format());
  }

  Format format() const noexcept override
  {
    return format_;
  }

  std::size_t size() const noexcept override
  {
    return size_;
  }

  bool is_empty() const noexcept override
  {
    return !size();
  }

  const void* bytes() const noexcept override
  {
    return storage_.get() ? storage_.get() : "";
  }

  Storage& storage() noexcept
  {
    return storage_;
  }

  const Storage& storage() const noexcept
  {
    return storage_;
  }

private:
  const Format format_{Format::text};
  std::size_t size_{};
  std::unique_ptr<T, Deleter> storage_;
};

/// The alias of Memory_data<char[]>.
using Array_data = Memory_data<char[]>;

/// The alias of Memory_data<void, void(*)(void*)>.
using Custom_data = Memory_data<void, void(*)(void*)>;

template<typename ... Types>
std::unique_ptr<Data> make_array_data(Types&& ... args)
{
  return std::make_unique<Array_data>(std::forward<Types>(args)...);
}

template<typename ... Types>
std::unique_ptr<Data> make_custom_data(Types&& ... args)
{
  return std::make_unique<Custom_data>(std::forward<Types>(args)...);
}

/**
 * @returns The result of conversion of text representation of the
 * PostgreSQL's Bytea data type to a plain binary data.
 *
 * @par Requires
 * `text != nullptr`. `text` must be a null-terminated C-string.
 *
 * @remarks bytes() of a resulting Data are not null-terminated.
 */
DMITIGR_PGFE_API std::unique_ptr<Data> make_bytea_data(const char* text);

// =============================================================================
// Type_data
// =============================================================================

/// A data of T.
template<typename T>
class Type_data : public Data {
public:
  /// An alias of the underlying type.
  using Type = T;

  template<typename ... Types>
  Type_data(Types&& ... args)
    : data_{std::forward<Types>(args)...}
  {}

  /// @see Data::to_data().
  std::unique_ptr<Data> to_data() const override
  {
    return std::make_unique<Type_data<Type>>(*this);
  }

  /// @see Data::format().
  Data_format format() const noexcept override
  {
    return Data_format::binary;
  }

  /// @see Data::size().
  std::size_t size() const noexcept override
  {
    return sizeof(Type);
  }

  /// @see Data::is_empty().
  bool is_empty() const noexcept override
  {
    return false;
  }

  /// @see Data::bytes().
  const void* bytes() const noexcept override
  {
    return std::addressof(data_);
  }

  /// @returns The underlying data.
  Type& data() noexcept
  {
    return data_;
  }

  /// @overload
  const Type& data() const noexcept
  {
    return data_;
  }

private:
  Type data_;
};

// =============================================================================
// Data_view
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
