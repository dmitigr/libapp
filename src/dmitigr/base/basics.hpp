// -*- C++ -*-
//
// Copyright 2026 Dmitry Igrishin
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

#ifndef DMITIGR_BASE_BASICS_HPP
#define DMITIGR_BASE_BASICS_HPP

namespace dmitigr {

/// The debug mode indicator.
#ifndef NDEBUG
constexpr bool is_debug{true};
#else
constexpr bool is_debug{false};
#endif

/// A throw mode.
enum class Throwable { no, yes };

} // namespace dmitigr

#endif  // DMITIGR_BASE_BASICS_HPP
