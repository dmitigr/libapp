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

#ifndef DMITIGR_PGFE_TUPLE_HPP
#define DMITIGR_PGFE_TUPLE_HPP

#include "composite.hpp"
#include "conversions_api.hpp"
#include "data.hpp"
#include "dll.hpp"
#include "exceptions.hpp"
#include "types_fwd.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace dmitigr::pgfe {

/**
 * @ingroup utilities
 *
 * @brief A tuple.
 *
 * @details A collection of elements in a fixed order.
 */
template<typename Elem>
class Tuple final : public Composite {
public:
  /// An alias of the tuple element.
  using Element = Elem;

  /// Default-constructible
  Tuple() = default;

  /// The constructor.
  Tuple(std::vector<Element>&& elements) noexcept
    : elements_{std::move(elements)}
  {
    assert(is_invariant_ok());
  }

  /// Copy-constructible.
  Tuple(const Tuple& rhs)
    : elements_{rhs.elements_.size()}
  {
    transform(rhs.elements_.cbegin(), rhs.elements_.cend(), elements_.begin(),
      [](const auto& pair)
      {
        return std::make_pair(pair.first, pair.second->to_data());
      });
    assert(is_invariant_ok());
  }

  /// Copy-assignable.
  Tuple& operator=(const Tuple& rhs)
  {
    if (this != &rhs) {
      Tuple tmp{rhs};
      swap(tmp);
    }
    return *this;
  }

  /// Move-constructible.
  Tuple(Tuple&& rhs) = default;

  /// Move-assignable.
  Tuple& operator=(Tuple&& rhs) = default;

  /// Swaps the instances.
  void swap(Tuple& rhs) noexcept
  {
    using std::swap;
    swap(elements_, rhs.elements_);
  }

  /// @see Compositional::field_count().
  std::size_t field_count() const noexcept override
  {
    return elements_.size();
  }

  /// @see Compositional::is_empty().
  bool is_empty() const noexcept override
  {
    return elements_.empty();
  }

  /// @see Compositional::field_name().
  std::string_view field_name(std::size_t index) const override
  {
    if (!(index < field_count()))
      throw Generic_exception{"cannot get field name of tuple"};
    return elements_[index].first;
  }

  /// @see Compositional::field_index().
  std::size_t field_index(std::string_view name,
    std::size_t offset = 0) const noexcept override
  {
    const std::size_t fc{field_count()};
    if (!(offset < fc))
      return fc;
    const auto b = elements_.cbegin();
    const auto e = elements_.cend();
    using Diff = typename decltype(b)::difference_type;
    const auto i = find_if(b + static_cast<Diff>(offset), e,
      [&name](const auto& pair) {return pair.first == name;});
    return static_cast<std::size_t>(i - b);
  }

  /**
   * @returns The field data of this tuple.
   *
   * @param index See Compositional.
   *
   * @par Requires
   * `index < field_count()`.
   */
  Data_view data(std::size_t index) const override
  {
    if (!(index < field_count()))
      throw Generic_exception{"cannot get data of tuple"};
    const auto& result = elements_[index].second;
    return result ? Data_view{*result} : Data_view{};
  }

  /**
   * @overload
   *
   * @par Requires
   * `has_field(name, offset)`.
   *
   * @see has_field(), Compositional::field_index().
   */
  Data_view data(std::string_view name,
    std::size_t offset = 0) const override
  {
    return data(field_index(name, offset));
  }

  /**
   * @brief Overwrites the field of this tuple with the value of type `T`.
   *
   * @par Requires
   * `index < field_count()`.
   */
  template<typename T>
  void set(const std::size_t index, T&& value)
  {
    if (!(index < field_count()))
      throw Generic_exception{"cannot set data of tuple"};
    elements_[index].second = to_data(std::forward<T>(value));
  }

  /// @overload
  template<typename T>
  void set(const std::string_view name, T&& value)
  {
    set(field_index(name), std::forward<T>(value));
  }

  /**
   * @brief Appends the field to this tuple.
   *
   * @param name A name of a new field.
   * @param value A value of a new field.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<typename T>
  void append(std::string name, T&& value)
  {
    elements_.emplace_back(std::move(name), to_data(std::forward<T>(value)));
    assert(is_invariant_ok());
  }

  /// Appends `rhs` to the end of the instance.
  void append(Tuple rhs)
  {
    elements_.insert(elements_.cend(),
      std::make_move_iterator(rhs.elements_.begin()),
      std::make_move_iterator(rhs.elements_.end()));
    assert(is_invariant_ok());
  }

  /**
   * @brief Inserts new field to this tuple.
   *
   * @param index An index of a field before which a new field will be inserted.
   * @param name A name of a new field.
   * @param value A value of a new field.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @par Requires
   * `index < field_count()`.
   */
  template<typename T>
  void insert(const std::size_t index, std::string name, T&& value)
  {
    if (!(index < field_count()))
      throw Generic_exception{"cannot insert field to tuple"};
    const auto b = elements_.begin();
    using Diff = typename decltype(b)::difference_type;
    elements_.insert(b + static_cast<Diff>(index),
      std::make_pair(std::move(name), to_data(std::forward<T>(value))));
    assert(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @param name A name of a field before which a new field will be inserted.
   * @param new_field_name A name of a new field.
   * @param value A value of a new field.
   *
   * @par Requires
   * `has_field(name, 0)`.
   */
  template<typename T>
  void insert(const std::string_view name, std::string new_field_name, T&& value)
  {
    insert(field_index(name), std::move(new_field_name),
      to_data(std::forward<T>(value)));
  }

  /**
   * @brief Removes field from this tuple.
   *
   * @par Requires
   * `index < field_count()`.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  void remove(std::size_t index)
  {
    if (!(index < field_count()))
      throw Generic_exception{"cannot remove field from tuple"};
    const auto b = elements_.cbegin();
    using Diff = typename decltype(b)::difference_type;
    elements_.erase(b + static_cast<Diff>(index));
    assert(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @par Effects
   * `!has_field(name, offset)`.
   *
   * @see has_field(), Compositional::field_index().
   */
  void remove(std::string_view name, std::size_t offset = 0)
  {
    if (const auto index = field_index(name, offset); index != field_count()) {
      const auto b = elements_.cbegin();
      using Diff = typename decltype(b)::difference_type;
      elements_.erase(b + static_cast<Diff>(index));
    }
    assert(is_invariant_ok());
  }

  /// @returns The underlying vector of elements.
  const std::vector<Element>& vector() const noexcept
  {
    return elements_;
  }

  /// @overload
  std::vector<Element>& vector() noexcept
  {
    return const_cast<std::vector<Element>&>(
      static_cast<const Tuple*>(this)->vector());
  }

private:
  std::vector<Element> elements_;
};

/**
 * @ingroup utilities
 *
 * @brief Tuple is swappable.
 */
template<typename Elem>
inline void swap(Tuple<Elem>& lhs, Tuple<Elem>& rhs) noexcept
{
  lhs.swap(rhs);
}

} // namespace dmitigr::pgfe

#endif  // DMITIGR_PGFE_TUPLE_HPP
