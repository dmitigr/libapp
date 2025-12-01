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

#ifndef DMITIGR_RAJSON_EMPLACE_HPP
#define DMITIGR_RAJSON_EMPLACE_HPP

#include "../base/assert.hpp"
#include "../base/error.hpp"

#include "errctg.hpp"
#include "path.hpp"
#include "rapidjson.hpp"

#include <utility>

namespace dmitigr::rajson {

/// Emplaces the `value` at the given `path` starting from `root`.
template<typename T>
Err emplace(rapidjson::Value& root, rapidjson::Document::AllocatorType& alloc,
  const Path& path, T&& value)
{
  if (!root.IsObject())
    return Err{Errc::value_not_object};

  auto val = to_value(std::forward<T>(value), alloc);
  auto* curr = &root;
  for (auto ci = path.components().cbegin(), ce = path.components().cend();
       ci != ce; ++ci) {
    const bool is_last{ci + 1 == ce};
    auto mi = curr->FindMember(*ci);
    if (mi == curr->MemberEnd()) {
      curr->AddMember(rapidjson::Value{*ci, alloc},
        !is_last ? rapidjson::Value{rapidjson::kObjectType}
                 : std::move(val), // create a node or leaf
        alloc);
      mi = curr->FindMember(*ci);
      DMITIGR_ASSERT(mi != curr->MemberEnd());
    } else if (is_last)
      mi->value = std::move(val);
    curr = &mi->value;
  }

  return Err{};
}

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_EMPLACE_HPP
