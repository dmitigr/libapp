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

#ifndef DMITIGR_BASE_STRING_VECTOR_HPP
#define DMITIGR_BASE_STRING_VECTOR_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace dmitigr {

template<class Str>
class String_vector final {
public:
  using String = Str;

  bool is_empty() const noexcept
  {
    return strings_.empty();
  }

  template<class Alloc = std::allocator<typename String::value_type>>
  auto to_string() const
  {
    std::basic_string<typename String::value_type,
      typename String::traits_type, Alloc> result;
    result.reserve(total_size_);
    for (const auto& string : strings_)
      result.append(string);
    return result;
  }

  std::size_t total_size() const noexcept
  {
    return total_size_;
  }

  const std::vector<String>& strings() const noexcept
  {
    return strings_;
  }

  void reserve(const std::size_t capacity)
  {
    strings_.reserve(capacity);
  }

  void push_back(String str)
  {
    total_size_ += str.size();
    strings_.push_back(std::move(str));
  }

private:
  std::size_t total_size_{};
  std::vector<String> strings_;
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_STRING_VECTOR_HPP
