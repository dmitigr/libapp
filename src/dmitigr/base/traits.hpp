// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
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

#ifndef DMITIGR_BASE_TRAITS_HPP
#define DMITIGR_BASE_TRAITS_HPP

#include <array>
#include <type_traits>

namespace dmitigr {

template<typename>
constexpr bool false_value{};

template<typename>
constexpr bool true_value{true};

template<typename>
struct Is_std_array : std::false_type {};

template<typename T, auto N>
struct Is_std_array<std::array<T, N>> : std::true_type {};

template<typename T>
constexpr bool Is_std_array_v = Is_std_array<T>::value;

} // namespace dmitigr

#endif  // DMITIGR_BASE_TRAITS_HPP
