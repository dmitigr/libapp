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

#ifndef DMITIGR_STR_WALKER_HPP
#define DMITIGR_STR_WALKER_HPP

#include <stdexcept>
#include <string_view>

namespace dmitigr::str {

template<class StringView>
class Basic_walker final {
public:
  using View = StringView;

  explicit Basic_walker(const View str, const View sep)
    : str_{str}
    , sep_{sep}
  {
    if (!sep_.data())
      throw std::invalid_argument{"invalid separator for str::Walker"};
  }

  View next() noexcept
  {
    if (offset_ != View::npos) {
      const auto pos = str_.find(sep_, offset_);
      if (pos != View::npos) {
        auto result = str_.substr(offset_, pos - offset_);
        offset_ = pos + 1 >= str_.size() ? View::npos : pos + 1;
        return result;
      } else {
        auto result = str_.substr(offset_);
        offset_ = View::npos;
        return result;
      }
    }
    return {};
  }

private:
  View str_;
  View sep_;
  typename View::size_type offset_{};
};

using Walker = Basic_walker<std::string_view>;
using Wwalker = Basic_walker<std::wstring_view>;

} // namespace dmitigr::str

#endif  // DMITIGR_STR_WALKER
