// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
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

#ifndef DMITIGR_TPL_GENERIC_HPP
#define DMITIGR_TPL_GENERIC_HPP

#include "../base/assert.hpp"
#include "../base/error.hpp"
#include "../base/ret.hpp"
#include "parameter.hpp"

#include <algorithm>
#include <iterator>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dmitigr::tpl {

/// A generic template.
class Generic final {
public:
  /// The default constructor.
  Generic() = default;

  /**
   * @brief Constructs the object by parsing the `input`.
   *
   * @details The `input` may contain parameters which can be binded with the
   * values by using Parameter::set_value() method. The parameter name *must* be
   * surrounded with left delimiter (*ldelim*) and right delimiter (*rdelim*).
   */
  static Ret<Generic> make(const std::string_view input,
    const std::string_view ldelim, const std::string_view rdelim)
  {
    Generic result;
    if (input.empty()) {
      DMITIGR_ASSERT(result.is_invariant_ok());
      return result;
    } else if (ldelim.empty() || rdelim.empty()) {
      result.fragments_.emplace_back(Fragment::text, std::string{input});
      DMITIGR_ASSERT(result.is_invariant_ok());
      return result;
    }

    enum { text, ldel, param, rdel } state = text;
    std::string_view::size_type dpos{};
    std::string extracted_text;
    std::string extracted_param;

    const auto store_extracted_text = [&result, &extracted_text]
    {
      result.fragments_.emplace_back(Fragment::text, std::move(extracted_text));
      extracted_text = {};
    };

    const auto store_extracted_param = [&result, &extracted_param]
    {
      if (!extracted_param.empty()) {
        // Since equally named parameters will share the same value,
        // we must check the presentence of the parameter in parameters_.
        if (const auto index = result.parameter_index(extracted_param); !index)
          result.parameters_.emplace_back(extracted_param, std::nullopt);

        result.fragments_.emplace_back(Fragment::parameter, std::move(extracted_param));
        extracted_param = {};
      }
    };

    for (const char ch : input) {
      switch (state) {
      case text:
        if (ch == ldelim[dpos]) {
          state = ldel;
          ++dpos;
          continue; // skip delim char
        } else
          break;

      case ldel:
        if (dpos >= ldelim.size()) {
          state = param;
          dpos = 0;
          extracted_param += ch;
          continue;
        } else if (ch != ldelim[dpos]) {
          state = text;
          extracted_text += ldelim.substr(0, dpos); // restore skipped chars
          dpos = 0;
          break;
        } else {
          ++dpos;
          continue; // skip delim char
        }

      case param:
        if (ch == rdelim[dpos]) {
          state = rdel;
          ++dpos;
          continue; // skip delim char
        } else {
          extracted_param += ch;
          continue; // c is already stored
        }

      case rdel:
        if (dpos >= rdelim.size()) {
          // Store the text preceding the parameter and the parameter itself.
          store_extracted_text();
          store_extracted_param();
          state = text;
          dpos = 0;
          break;
        } else if (ch != rdelim[dpos]) {
          state = text;
          extracted_text += ldelim;
          extracted_text += extracted_param;
          extracted_text += rdelim.substr(0, dpos); // restore skipped chars
          extracted_param.clear();
          dpos = 0;
          break;
        } else {
          ++dpos;
          continue; // skip delim char
        }
      }

      extracted_text += ch;
    }

    if (state == rdel && dpos >= rdelim.size()) {
      store_extracted_text();
      store_extracted_param();
    } else if (!extracted_param.empty())
      extracted_text.append(ldelim).append(extracted_param);

    if (!extracted_text.empty())
      store_extracted_text();

    DMITIGR_ASSERT(result.is_invariant_ok());
    return result;
  }

  /// @returns The vector of parameters.
  const std::vector<Parameter>& parameters() const noexcept
  {
    return parameters_;
  }

  /// @returns The vector of unbound parameter names.
  std::vector<std::string> unbound_parameter_names() const
  {
    std::vector<std::string> result;
    for (const auto& p : parameters_) {
      if (!p.value())
        result.push_back(p.name());
    }
    return result;
  }

  /// @returns The number of parameters.
  std::size_t parameter_count() const noexcept
  {
    return parameters_.size();
  }

  /// @returns The parameter index if `has_parameter(name)`.
  std::optional<std::size_t>
  parameter_index(const std::string_view name) const noexcept
  {
    const auto b = cbegin(parameters_);
    const auto e = cend(parameters_);
    const auto i = find_if(b, e, [&](const auto& p)
    {
      return p.name() == name;
    });
    return i != e ? std::make_optional(i - b) : std::nullopt;
  }

  /// @returns The parameter or `nullptr`.
  const Parameter* parameter(const std::size_t index) const noexcept
  {
    return index < parameter_count() ? &parameters_[index] : nullptr;
  }

  /// @overload
  Parameter* parameter(const std::size_t index) noexcept
  {
    return const_cast<Parameter*>(
      static_cast<const Generic*>(this)->parameter(index));
  }

  /**
   * @brief Binds the parameter at index `index` with the `value`.
   *
   * @returns `true` on success.
   */
  bool bind(const std::size_t index, std::optional<std::string> value) noexcept
  {
    auto* const p = parameter(index);
    if (p)
      p->set_value(std::move(value));
    return p;
  }

  /**
   * @returns The parameter or `nullptr`.
   */
  const Parameter* parameter(const std::string_view name) const noexcept
  {
    if (const auto index = parameter_index(name))
      return &parameters_[*index];
    else
      return nullptr;
  }

  /// @overload
  Parameter* parameter(const std::string_view name) noexcept
  {
    return const_cast<Parameter*>(
      static_cast<const Generic*>(this)->parameter(name));
  }

  /**
   * @brief Binds the parameter at index `index` with the `value`.
   *
   * @returns `true` on success.
   */
  bool bind(const std::string_view name, std::optional<std::string> value) noexcept
  {
    auto* const p = parameter(name);
    if (p)
      p->set_value(std::move(value));
    return p;
  }

  /// @returns `true` if the parameter named by `name` is presents.
  bool has_parameter(const std::string_view name) const noexcept
  {
    return static_cast<bool>(parameter_index(name));
  }

  /// @returns `(parameter_count() > 0)`.
  bool has_parameters() const noexcept
  {
    return !parameters_.empty();
  }

  /**
   * @returns `true` if this instance has the parameter with the index `i`
   * such that `!parameter(i).value()`.
   *
   * @see parameter().
   */
  bool has_unbound_parameters() const noexcept
  {
    return any_of(cbegin(parameters_), cend(parameters_),
      [](const auto& p) { return !p.value(); });
  }

  /**
   * @brief Replaces the parameter at `index` with the specified `tpl`.
   *
   * @par Requires
   * `index < parameter_count() && (&tpl != this)`.
   *
   * @par Effects
   * This instance contains the given `tpl` instead of the parameter at `index`.
   * Parameter indices greater than `index` are likely changed.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see has_parameter().
   */
  Err replace_parameter(const std::size_t index, const Generic& tpl)
  {
    if (!(index < parameter_count()))
      return Err{Errc::generic, "text template parameter index " +
        std::to_string(index) + " is out of range [0, " +
        std::to_string(parameter_count()) + ")"};
    else if (&tpl == this)
      return Err{Errc::generic, "cannot replace text template parameter of index "
        + std::to_string(index) + " with the text template itself"};

    // Borrowing fragments.
    {
      std::size_t i{};
      for (auto fi = begin(fragments_); fi != end(fragments_);) {
        if (fi->first == Fragment::parameter) {
          if (i == index) {
            fragments_.insert(fi, cbegin(tpl.fragments_), cend(tpl.fragments_));
            fragments_.erase(fi);
            break;
          } else
            ++i;
        }
        ++fi;
      }
      DMITIGR_ASSERT(i == index);
    }

    // Remove parameters from the original which are in `tpl`.
    for (const auto& p : tpl.parameters_) {
      if (const auto i = parameter_index(p.name()))
        parameters_.erase(begin(parameters_) + *i);
    }

    // Borrow parameters from the `tpl`.
    const auto itr = parameters_.erase(cbegin(parameters_) + index);
    parameters_.insert(itr, cbegin(tpl.parameters_), end(tpl.parameters_));

    DMITIGR_ASSERT(is_invariant_ok());
    return Err{};
  }

  /**
   * @brief Replaces the parameter named by the `name` with the specified `tpl`.
   *
   * @par Requires
   * `has_parameter(name) && (&tpl != this)`.
   *
   * @par Effects
   * This instance contains the given `tpl` instead of the parameter named by
   * the `name`. Parameter indices greater than parameter_index(name) are
   * likely changed.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see has_parameter().
   */
  Err replace_parameter(const std::string_view name, const Generic& tpl)
  {
    if (!has_parameter(name))
      return Err{Errc::generic,
        std::string{"cannot replace missing text template"}
        .append(" parameter \"").append(name).append("\"")};
    else if (&tpl == this)
      return Err{Errc::generic,
        std::string{"cannot replace text template"}
        .append(" parameter \"").append(name)
        .append("\" with text template itself")};

    // Borrowing fragments.
    for (auto fi = begin(fragments_); fi != end(fragments_);) {
      if (fi->first == Fragment::parameter && fi->second == name) {
        // 1. Insert the `tpl` just before `fi`.
        fragments_.insert(fi, cbegin(tpl.fragments_), cend(tpl.fragments_));
        // 2. Erase the parameter pointed by `fi` and get the next iterator.
        fi = fragments_.erase(fi);
      } else
        ++fi;
    }

    // Remove parameters from the original which are in `tpl`.
    for (const auto& p : tpl.parameters_) {
      if (const auto i = parameter_index(p.name()))
        parameters_.erase(begin(parameters_) + *i);
    }

    // Borrow parameters from the `tpl`.
    const auto idx = parameter_index(name);
    DMITIGR_ASSERT(idx);
    const auto itr = parameters_.erase(cbegin(parameters_) + *idx);
    parameters_.insert(itr, cbegin(tpl.parameters_), end(tpl.parameters_));

    DMITIGR_ASSERT(is_invariant_ok());
    return Err{};
  }

  /// Shortcut of replace_parameter().
  Err replace(const std::size_t index, const Generic& tpl)
  {
    return replace_parameter(index, tpl);
  }

  /// @overload
  Err replace(const std::string_view name, const Generic& tpl)
  {
    return replace_parameter(name, tpl);
  }

  /// Appends `apx` to this instance.
  void append(Generic apx)
  {
    // Append fragments.
    fragments_.insert(cend(fragments_),
      make_move_iterator(cbegin(apx.fragments_)),
      make_move_iterator(cend(apx.fragments_)));

    // Append parameters.
    for (auto& p : apx.parameters_) {
      if (!has_parameter(p.name()))
        parameters_.emplace_back(std::move(p.name_), std::move(p.value_));
    }
  }

  /// @name Conversions
  /// @{

  /**
   * @returns The result of conversion of this instance to the instance of
   * type `std::string`.
   */
  std::string to_string(const std::string_view ldelim,
    const std::string_view rdelim) const
  {
    std::string result;
    for (const auto& fragment : fragments_) {
      switch (fragment.first) {
      case Fragment::text:
        result.append(fragment.second);
        break;
      case Fragment::parameter:
        result.append(ldelim).append(fragment.second).append(rdelim);
        break;
      }
    }
    return result;
  }

  /**
   * @returns The output string.
   *
   * @par Requires
   * `!has_unbound_parameters()`.
   */
  Ret<std::string> to_output() const
  {
    std::string result;
    for (const auto& fragment : fragments_) {
      const auto& name = fragment.second;
      switch (fragment.first) {
      case Fragment::text:
        result.append(name);
        break;
      case Fragment::parameter: {
        const auto* const p = parameter(name);
        DMITIGR_ASSERT(p);
        if (const auto& value = p->value())
          result.append(value.value());
        else
          return Err{Errc::generic, "text template parameter \""+name+"\" unbound"};
        break;
      }
      }
    }
    return result;
  }

  /// @}

  /// @name Comparisons
  /// @{

  /// @returns `true` if `*this` is less than `rhs`.
  bool operator<(const Generic& rhs) const noexcept
  {
    return fragments_ < rhs.fragments_ && parameters_ < rhs.parameters_;
  }

  /// @returns `true` if `*this` is less or equals to `rhs`.
  bool operator<=(const Generic& rhs) const noexcept
  {
    return fragments_ <= rhs.fragments_ && parameters_ <= rhs.parameters_;
  }

  /// @returns `true` if `*this` is equals to `rhs`.
  bool operator==(const Generic& rhs) const noexcept
  {
    return fragments_ == rhs.fragments_ && parameters_ == rhs.parameters_;
  }

  /// @returns `true` if `*this` is not equals to `rhs`.
  bool operator!=(const Generic& rhs) const noexcept
  {
    return fragments_ != rhs.fragments_ && parameters_ != rhs.parameters_;
  }

  /// @returns `true` if `*this` is greater or equals to `rhs`.
  bool operator>=(const Generic& rhs) const noexcept
  {
    return fragments_ >= rhs.fragments_ && parameters_ >= rhs.parameters_;
  }

  /// @returns `true` if `*this` is greater than `rhs`.
  bool operator>(const Generic& rhs) const noexcept
  {
    return fragments_ > rhs.fragments_ && parameters_ > rhs.parameters_;
  }

  /// @}

private:
  enum class Fragment { text, parameter };

  std::list<std::pair<Fragment, std::string>> fragments_;
  std::vector<Parameter> parameters_;

  bool is_invariant_ok() const noexcept
  {
    return parameters_.empty() || !fragments_.empty();
  }
};

} // namespace dmitigr::tpl

#endif  // DMITIGR_TPL_GENERIC_HPP
