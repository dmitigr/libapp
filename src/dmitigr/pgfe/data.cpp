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

#include "data.hpp"
#include "exceptions.hpp"
#include "pq.hpp"

#include <algorithm> // swap
#include <cstring>
#include <new> // bad_alloc

namespace dmitigr::pgfe {

// =============================================================================
// Data
// =============================================================================

DMITIGR_PGFE_INLINE bool Data::is_valid() const noexcept
{
  return static_cast<int>(format()) >= 0;
}

DMITIGR_PGFE_INLINE std::strong_ordering
operator<=>(const Data& lhs, const Data& rhs) noexcept
{
  const auto lfm = lhs.format(), rfm = rhs.format();
  const auto lsz = lhs.size(), rsz = rhs.size();
  if (lfm == rfm && lsz == rsz) {
    const auto r = std::memcmp(lhs.bytes(), rhs.bytes(), lsz);
    return r < 0 ? std::strong_ordering::less :
      r > 0 ? std::strong_ordering::greater : std::strong_ordering::equal;
  } else if (lfm < rfm)
    return std::strong_ordering::less;
  else if (lfm > rfm)
    return std::strong_ordering::greater;
  else
    return lsz < rsz ? std::strong_ordering::less : std::strong_ordering::greater;
}

DMITIGR_PGFE_INLINE bool
operator==(const Data& lhs, const Data& rhs) noexcept
{
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

// =============================================================================
// Memory_data
// =============================================================================

DMITIGR_PGFE_INLINE std::unique_ptr<Data> make_bytea_data(const char* const text)
{
  if (!text)
    throw Generic_exception{"cannot make bytea data: null text"};

  const auto* const bytes = reinterpret_cast<const unsigned char*>(text);
  std::size_t storage_size{};
  using Uptr = std::unique_ptr<void, void(*)(void*)>;
  if (auto storage = Uptr{PQunescapeBytea(bytes, &storage_size), &PQfreemem})
    return make_custom_data(std::move(storage), storage_size, Data_format::binary);
  else
    throw std::bad_alloc{};
}

// =============================================================================
// Data_view
// =============================================================================

DMITIGR_PGFE_INLINE Data_view::Data_view(const char* const bytes) noexcept
{
  if (bytes) {
    format_ = Format::text;
    data_ = bytes;
  }
}

DMITIGR_PGFE_INLINE Data_view::Data_view(const char* const bytes,
  const std::size_t size, const Format format) noexcept
{
  if (bytes) {
    format_ = format;
    data_ = {bytes, size};
  }
}

DMITIGR_PGFE_INLINE Data_view::Data_view(const Data& data) noexcept
  : Data_view{static_cast<const char*>(data.bytes()), data.size(), data.format()}
{}

DMITIGR_PGFE_INLINE Data_view::Data_view(Data_view&& rhs) noexcept
  : format_{rhs.format_}
  , data_{std::move(rhs.data_)}
{
  rhs.format_ = Format{-1};
}

DMITIGR_PGFE_INLINE Data_view& Data_view::operator=(Data_view&& rhs) noexcept
{
  if (this != &rhs) {
    Data_view tmp{std::move(rhs)};
    swap(tmp);
  }
  return *this;
}

DMITIGR_PGFE_INLINE void Data_view::swap(Data_view& rhs) noexcept
{
  using std::swap;
  swap(format_, rhs.format_);
  swap(data_, rhs.data_);
}

DMITIGR_PGFE_INLINE std::unique_ptr<Data> Data_view::to_data() const
{
  return make_string_data(std::string(data_.data(), data_.size()), format_);
}

DMITIGR_PGFE_INLINE auto Data_view::format() const noexcept -> Format
{
  return format_;
}

DMITIGR_PGFE_INLINE std::size_t Data_view::size() const noexcept
{
  return data_.size();
}

DMITIGR_PGFE_INLINE bool Data_view::is_empty() const noexcept
{
  return data_.empty();
}

DMITIGR_PGFE_INLINE const void* Data_view::bytes() const noexcept
{
  return data_.data();
}

} // namespace dmitigr::pgfe
