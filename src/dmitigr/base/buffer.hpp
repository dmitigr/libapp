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

#ifndef DMITIGR_BASE_BUFFER_HPP
#define DMITIGR_BASE_BUFFER_HPP

#include "assert.hpp"
#include "noncopymove.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>

namespace dmitigr {

/// A Buffer traits.
template<typename B>
struct Buffer_traits final {
  /// A buffer.
  using Buffer = B;

  /// A buffer data.
  using Data = std::decay_t<decltype(*Buffer::data)>;

  /// Allocates the memory of required `capacity` to store Data values.
  static Data* allocate(const std::size_t capacity) noexcept
  {
    return capacity ? static_cast<Data*>(std::malloc(sizeof(Data)*capacity)) : nullptr;
  }

  /// Deallocates the memory to store Data values.
  static void deallocate(Data* const data) noexcept
  {
    if (data)
      std::free(data);
  }

  /// Reallocates the memory of `buf`.
  static void reallocate(Buffer* const buf, const std::size_t capacity) noexcept
  {
    if (buf) {
      if (buf->data)
        std::free(buf->data);
      buf->data = allocate(capacity);
      buf->size = 0;
      buf->capacity = buf->data ? capacity : 0;
    }
  }

  /// Initializes the `buf` by allocating the memory of required `capacity`.
  static void initialize(Buffer* const buf, const std::size_t capacity) noexcept
  {
    if (buf) {
      buf->data = allocate(capacity);
      buf->size = 0;
      buf->capacity = buf->data ? capacity : 0;
    }
  }

  /// Deinitializes the `buf` by deallocating the memory of `buf`.
  static void deinitialize(Buffer* const buf) noexcept
  {
    if (buf) {
      if (buf->data) {
        deallocate(buf->data);
        buf->data = nullptr;
      }
      buf->size = 0;
      buf->capacity = 0;
    }
  }

  /**
   * @brief If `buf->capacity >= capacity` does nothing. Otherwise, allocates
   * the memory of the specified `capacity`, copies `data->data` to this newly
   * allocated memory, deallocates the `buf->data` and  assigns the new memory
   * space and `capacity` to the `buf->data` and `buf->capacity` accordingly.
   *
   * @returns `0` on success.
   */
  static int reserve(Buffer* const buf, const std::size_t capacity) noexcept
  {
    if (buf) {
      if (buf->data && buf->capacity >= capacity)
        return 0;
      Buffer new_buffer;
      initialize(&new_buffer, capacity);
      if (new_buffer.data) {
        std::memcpy(new_buffer.data, buf->data, sizeof(Data)*buf->size);
        deallocate(buf->data);
        buf->data = new_buffer.data;
        buf->capacity = new_buffer.capacity;
        DMITIGR_ASSERT(buf->capacity >= capacity);
        return 0;
      }
    }
    return -1;
  }

  /**
   * @brief Calls reserve(size) and assigns `size` to the `buf->size`.
   *
   * @returns `0` on success.
   */
  static int resize(Buffer* const buf, const std::size_t size) noexcept
  {
    if (buf) {
      if (!reserve(buf, size)) {
        buf->size = size;
        return 0;
      }
    }
    return -1;
  }

  /**
   * @brief If `buf->capacity >= capacity` does nothing. Otherwise, reallocates
   * the memory of `buf->data` to fit the required capacity.
   *
   * @returns `0` on success.
   */
  static int destructive_reserve(Buffer* const buf, const std::size_t capacity) noexcept
  {
    if (buf) {
      if (buf->data && buf->capacity >= capacity)
        return 0;
      reallocate(buf, capacity);
      if (buf->data) {
        DMITIGR_ASSERT(buf->capacity >= capacity);
        return 0;
      }
    }
    return -1;
  }

  /**
   * @brief Calls destructive_reserve(buf, size) and assigns `size` to the
   * `buf->size`.
   *
   * @remarks This is useful for effictivelly preparing the `buf` to store `size`
   * elements without copying the current content into a newly allocated memory.
   *
   * @returns `0` on success.
   */
  static int destructive_resize(Buffer* const buf, const std::size_t size) noexcept
  {
    if (!destructive_reserve(buf, size)) {
      buf->size = size;
      return 0;
    }
    return -1;
  }

  /**
   * @brief Copies `data` into `buf->data` and assigns the `size` to `buf->size`.
   *
   * @returns `0` on success.
   */
  static int assign(Buffer* const buf,
    const Data* const data, const std::size_t size) noexcept
  {
    if (!destructive_resize(buf, size)) {
      std::memcpy(buf->data, data, sizeof(Data)*size);
      return 0;
    }
    return -1;
  }

  /// @returns The size of `buf`.
  static std::size_t size(const Buffer* const buf) noexcept
  {
    return buf->size;
  }

  /// @returns The capacity of `buf`.
  static std::size_t capacity(const Buffer* const buf) noexcept
  {
    return buf->capacity;
  }

  /// @returns The data of `buf`.
  static Data* data(const Buffer* const buf) noexcept
  {
    return buf->data;
  }
};

/// A RAII wrapper around the underlying `Buf`.
template<class Buf, class Tr = Buffer_traits<Buf>>
class Basic_buffer final : Noncopy {
public:
  /// An underlying buffer.
  using Buffer = Buf;

  /// Traits.
  using Traits = Tr;

  /// Deinitializes the underlying buffer.
  ~Basic_buffer()
  {
    Traits::deinitialize(&buffer_);
  }

  /// Constructs invalid instance (with null underlying buffer).
  Basic_buffer() = default;

  /// Constructs from `buf`.
  Basic_buffer(Buffer buf) noexcept
    : buffer_{buf}
  {}

  /// Allocates the memory for the underlying buffer.
  explicit Basic_buffer(const std::size_t capacity)
  {
    Traits::initialize(&buffer_, capacity);
    if (!*this)
      throw std::bad_alloc{};
  }

  /// Allocates the memory for the underlying buffer and copies `data` into it.
  Basic_buffer(const typename Traits::Data* const data, const std::size_t size)
  {
    assign(data, size);
  }

  /// Movable.
  Basic_buffer(Basic_buffer&& rhs) noexcept
    : buffer_{rhs.buffer}
  {
    rhs.buffer = {};
  }

  /// Move-assignable.
  Basic_buffer& operator=(Basic_buffer&& rhs) noexcept
  {
    Basic_buffer tmp{rhs.buffer_};
    swap(tmp);
    return *this;
  }

  /// Swaps `rhs` with `*this`.
  void swap(Basic_buffer& rhs) noexcept
  {
    using std::swap;
    swap(buffer_, rhs.buffer_);
  }

  /// @returns `true` if this instance is valid.
  explicit operator bool() const noexcept
  {
    return static_cast<bool>(Traits::data(&buffer_));
  }

  /// Allocates the memory for the underlying buffer and copies `data` into it.
  void assign(const typename Traits::Data* const data, const std::size_t size)
  {
    if (Traits::assign(&buffer_, data, size))
      throw std::bad_alloc{};
  }

  /**
   * @returns The underlying buffer.
   *
   * @par Effects
   * `!*this`.
   */
  Buffer release() noexcept
  {
    Buffer result;
    swap(result);
    return result;
  }

  /// @see Traits::reserve().
  void reserve(const std::size_t capacity)
  {
    if (Traits::reserve(&buffer_, capacity))
      throw std::bad_alloc{};
  }

  /// @see Traits::resize().
  void resize(const std::size_t size)
  {
    if (Traits::resize(&buffer_, size))
      throw std::bad_alloc{};
  }

  /// @see Traits::destructive_reserve().
  void destructive_reserve(const std::size_t capacity)
  {
    if (Traits::destructive_reserve(&buffer_, capacity))
      throw std::bad_alloc{};
  }

  /// @see Traits::destructive_resize().
  void destructive_resize(const std::size_t size)
  {
    if (Traits::destructive_resize(&buffer_, size))
      throw std::bad_alloc{};
  }

  /// @returns The underlying buffer.
  const Buffer& buffer() const noexcept
  {
    return buffer_;
  }

  /// @overload
  Buffer& buffer() noexcept
  {
    return buffer_;
  }

  /// @see Traits::size().
  std::size_t size() const noexcept
  {
    return Traits::size(&buffer_);
  }

  /// @see Traits::capacity().
  std::size_t capacity() const noexcept
  {
    return Traits::capacity(&buffer_);
  }

  /// @see Traits::data().
  const auto* data() const noexcept
  {
    return Traits::data(&buffer_);
  }

  /// @overload
  auto* data() noexcept
  {
    return const_cast<typename Traits::Data*>(
      static_cast<const Basic_buffer*>(this)->data());
  }

private:
  Buffer buffer_{};
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_BUFFER_HPP
