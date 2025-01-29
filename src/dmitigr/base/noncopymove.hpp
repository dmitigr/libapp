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

#ifndef DMITIGR_BASE_NONCOPYMOVE_HPP
#define DMITIGR_BASE_NONCOPYMOVE_HPP

namespace dmitigr {

class Noncopy {
public:
  Noncopy(const Noncopy&) = delete;
  Noncopy& operator=(const Noncopy&) = delete;
  Noncopy(Noncopy&&) = default;
  Noncopy& operator=(Noncopy&&) = default;
protected:
  Noncopy() = default;
};

class Nonmove {
public:
  Nonmove(const Nonmove&) = default;
  Nonmove& operator=(const Nonmove&) = default;
  Nonmove(Nonmove&&) = delete;
  Nonmove& operator=(Nonmove&&) = delete;
protected:
  Nonmove() = default;
};

class Noncopymove : Noncopy, Nonmove {
protected:
  Noncopymove() = default;
};

} // namespace dmitigr

#endif  // DMITIGR_BASE_NONCOPYMOVE_HPP
