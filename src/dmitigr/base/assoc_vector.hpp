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

#ifndef DMITIGR_BASE_ASSOC_VECTOR_HPP
#define DMITIGR_BASE_ASSOC_VECTOR_HPP

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dmitigr {

/// An associative vector of elements.
template<typename K, typename V>
class Assoc_vector final {
public:
  /// An alias of the key.
  using Key = K;

  /// An alias of the value.
  using Value = V;

  /// An alias of the element.
  using Element = std::pair<Key, Value>;

  /// An alias of the underlying type.
  using Underlying_type = std::vector<Element>;

  /// An alias of the allocator.
  using Allocator = typename Underlying_type::allocator_type;

  /// An alias of the size type.
  using Size = typename Underlying_type::size_type;

  /// Default-constructible
  Assoc_vector() = default;

  /// The constructor.
  explicit Assoc_vector(const Size size, const Allocator& alloc = Allocator())
    : elements_(size, alloc)
  {}

  /// The constructor.
  Assoc_vector(std::vector<Element>&& elements) noexcept
    : elements_{std::move(elements)}
  {}

  /// Swaps `*this` with `rhs`.
  void swap(Assoc_vector& rhs) noexcept
  {
    using std::swap;
    swap(elements_, rhs.elements_);
  }

  /// @returns The result of comparison `lhs` with `rhs`.
  auto operator<=>(const Assoc_vector& rhs) const noexcept
  {
    return elements_ < rhs.elements_ ? std::strong_ordering::less :
      elements_ > rhs.elements_ ? std::strong_ordering::greater :
      std::strong_ordering::equal;
  }

  /// @returns `(lhs <=> rhs) == 0`.
  bool operator==(const Assoc_vector& rhs) const noexcept
  {
    return operator<=>(rhs) == std::strong_ordering::equal;
  }

  /// @returns The number of elements.
  Size size() const noexcept
  {
    return elements_.size();
  }

  /// @returns `size() == 0`.
  bool is_empty() const noexcept
  {
    return elements_.empty();
  }

  /**
   * @returns The index of the element, or `size()` if no element found.
   *
   * @param key The key.
   * @param offset The offset to start search.
   */
  template<typename KeyType>
  Size index(const KeyType& key, const Size offset = 0) const noexcept
  {
    if (const auto sz = size(); !(offset < sz))
      return sz;
    const auto b = elements_.cbegin();
    const auto e = elements_.cend();
    using Diff = typename decltype(b)::difference_type;
    const auto i = find_if(b + static_cast<Diff>(offset), e,
      [&key](const auto& pair) {return pair.first == key;});
    return static_cast<std::size_t>(i - b);
  }

  /**
   * @returns The value by `key`.
   *
   * @par Requires
   * `index(key, offset) < size()`.
   */
  template<typename KeyType>
  const Value& value(const KeyType& key, const Size offset = 0) const
  {
    const auto idx = index(key, offset);
    if (!(idx < size()))
      throw std::invalid_argument{"cannot get value of dmitigr::Assoc_vector"};
    return elements_[idx].second;
  }

  /// @overload
  template<typename KeyType>
  Value& value(const KeyType& key, const Size offset = 0)
  {
    return const_cast<Value&>(
      static_cast<const Assoc_vector*>(this)->value(key, offset));
  }

  /**
   * @brief Calls `callback` for each element with key `key`.
   *
   * @param callback A function with signature `bool(Value&& value, Size index)`.
   * @param key A key, for the associated values of which the `callback` is called.
   */
  template<typename F, typename KeyType>
  void for_each(F&& callback, const KeyType& key) const
  {
    for_each(std::forward<F>(callback), elements_, key);
  }

  /// @overload
  template<typename F, typename KeyType>
  void for_each(F&& callback, const KeyType& key)
  {
    for_each(std::forward<F>(callback), elements_, key);
  }

  /**
   * @brief Appends the key-value pair to this vector.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<typename T>
  void emplace_back(Key key, T&& value)
  {
    elements_.emplace_back(std::move(key), std::forward<T>(value));
  }

  /// Appends `rhs` to this vector.
  void append(Assoc_vector rhs)
  {
    elements_.insert(elements_.cend(),
      std::make_move_iterator(rhs.elements_.begin()),
      std::make_move_iterator(rhs.elements_.end()));
  }

  /**
   * @brief Inserts the key-value pair before an element at index `index`.
   *
   * @par Requires
   * `index < size()`.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<typename T>
  void insert(const std::size_t index, Key key, T&& value)
  {
    if (!(index < size()))
      throw std::invalid_argument{"cannot insert key-value pair to"
        " dmitigr::Assoc_vector"};
    const auto b = elements_.begin();
    using Diff = typename decltype(b)::difference_type;
    elements_.insert(b + static_cast<Diff>(index),
      {std::move(key), std::forward<T>(value)});
  }

  /**
   * @brief Removes the key-value pair at `index` from this vector.
   *
   * @par Requires
   * `index < size()`.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  void remove(const std::size_t index)
  {
    if (!(index < size()))
      throw std::invalid_argument{"cannot remove key-value pair from"
        " dmitigr::Assoc_vector"};
    const auto b = elements_.cbegin();
    using Diff = typename decltype(b)::difference_type;
    elements_.erase(b + static_cast<Diff>(index));
  }

  /**
   * @brief Removes all key-value pairs with key `key` from this vector.
   *
   * @returns The number of removed elements.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<class KeyType>
  Size remove_each(const KeyType& key)
  {
    const auto cur_end = elements_.end();
    const auto new_end = remove_if(elements_.begin(), cur_end,
      [&key](const auto& pair) noexcept
      {
        return pair.first == key;
      });
    const auto result = cur_end - new_end;
    elements_.erase(new_end, cur_end);
    return result;
  }

  /// Clears this vector.
  void clear()
  {
    elements_.clear();
  }

  /// @returns The underlying vector.
  const Underlying_type& vector() const noexcept
  {
    return elements_;
  }

  /// @overload
  Underlying_type& vector() noexcept
  {
    return const_cast<std::vector<Element>&>(
      static_cast<const Assoc_vector*>(this)->vector());
  }

private:
  std::vector<Element> elements_;

  template<typename F, class E, typename KeyType>
  static void for_each(F&& callback, E&& elements, const KeyType& key)
  {
    const Size sz = elements.size();
    for (Size i{}; i < sz; ++i) {
      auto& [ky, value] = elements[i];
      if (ky == key && !callback(value, i))
        break;
    }
  }
};

/// Swaps `lhs` and `rhs`.
template<typename K, typename V>
void swap(Assoc_vector<K, V>& lhs, Assoc_vector<K, V>& rhs) noexcept
{
  lhs.swap(rhs);
}

} // namespace dmitigr

#endif  // DMITIGR_BASE_ASSOC_VECTOR_HPP
