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

#if !defined(__linux__)
#error dmitigr/nix/cpu.hpp is usable only on Linux!
#endif

#include "../base/assert.hpp"
#include "../base/str.hpp"
#include "../base/stream.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef DMITIGR_NIX_CPU_HPP
#define DMITIGR_NIX_CPU_HPP

namespace dmitigr::nix {

/// Represents SMT status.
enum class Smt_status {
  notsupported,
  forceoff,
  off,
  on
};

/// @returns The text representation of `value`.
inline const char* to_literal(const Smt_status value) noexcept
{
  using enum Smt_status;
  switch (value) {
  case notsupported:
    return "notsupported";
  case forceoff:
    return "forceoff";
  case off:
    return "off";
  case on:
    return "on";
  }
  return nullptr;
}

/// @returns The text representation of `value`.
inline std::string_view to_string_view(const Smt_status value)
{
  if (const auto* const result = to_literal(value))
    return result;
  DMITIGR_THROW_INVARG(value);
}

/// @returns The binary representation of `value`.
inline Smt_status to_smt_status(const std::string_view value)
{
  using enum Smt_status;
  if (value == "notsupported")
    return notsupported;
  else if (value == "forceoff")
    return forceoff;
  else if (value == "off")
    return off;
  else if (value == "on")
    return on;
  DMITIGR_THROW_INVARG(value);
}

/// @returns The status of Simultaneous Multithreading.
inline Smt_status smt_status()
{
  if (std::ifstream file{"/sys/devices/system/cpu/smt/control"})
    return to_smt_status(read_line_to_string(file));
  return Smt_status::notsupported;
}

/**
 * @brief A physical or logical CPU.
 */
class Cpu final {
public:
  /// A CPU range to describe topology.
  class Range final {
  public:
    /// Constructs invalid instance.
    Range() = default;

    /// Constructs the range which represents a single CPU core.
    explicit Range(const int index)
      : Range{index, index}
    {}

    /// Constructs the range which represents multiple CPU cores.
    Range(const int lower, const int upper)
      : lower_{lower}
      , upper_{upper}
    {
      DMITIGR_CKARG(lower_ <= upper_);
    }

    /// @returns `true` if this instance is valid.
    bool is_valid() const noexcept
    {
      return lower_ > 0 && upper_ > 0;
    }

    /// @returns `true` if this instance represents a single CPU core.
    bool is_single() const noexcept
    {
      return lower_ == upper_;
    }

    /// @returns A lower bound of the range.
    int lower() const noexcept
    {
      return lower_;
    }

    /// @returns An upper bound of the range.
    int upper() const noexcept
    {
      return upper_;
    }

    /// @returns A string representation of the range.
    std::string to_string() const
    {
      using std::to_string;
      return !is_single() ? to_string(lower()).append(1, '-')
        .append(to_string(upper())) : to_string(lower());
    }

  private:
    int lower_{-1};
    int upper_{-1};
  };

  /// Constructs invalid instance.
  Cpu() = default;

  /// Constructs an instance of CPU at index `index`.
  explicit Cpu(const int index)
    : index_{index}
  {
    DMITIGR_CKARG(is_possible(index));
  }

  /// @returns An instance of CPU at index `index` or `std::nullopt`.
  static std::optional<Cpu> make(const int index)
  {
    Cpu result{index, int{}};
    if (exists(result.system_path()))
      return std::optional{std::move(result)};
  }

  /// @returns `true` if this instance is valid.
  bool is_valid() const noexcept
  {
    return index() >= 0;
  }

  /// @returns `true` if Simultaneous Multithreading is available.
  bool is_smt_available() const
  {
    return core_list().size() > 1;
  }

  /// @returns `true` if this CPU core is presents.
  bool is_possible() const
  {
    static const std::filesystem::path file{cpu_root_path()/"possible"};
    return !exists(file) || is_in_ranges(file, index());
  }

  /// @returns `true` if this CPU core is online.
  bool is_online() const
  {
    static const std::filesystem::path file{cpu_root_path()/"online"};
    return !exists(file) || is_in_ranges(file, index());
  }

  /// @returns `true` if this CPU core is physical.
  bool is_physical() const noexcept
  {
    const auto list = core_list();
    return list.empty() || list.front().lower() == index();
  }

  /// @returns `true` if this CPU core is performant (P-core).
  bool is_performant() const
  {
    static const std::filesystem::path file{"/sys/devices/cpu_core/cpus"};
    return !exists(cpus) || is_in_ranges(file, index());
  }

  /// @returns An index of this CPU.
  int index() const noexcept
  {
    return index_;
  }

  /// @returns Max CPU capacity.
  static constexpr int max_capacity() noexcept
  {
    return 1024;
  }

  /// @returns CPU capacity.
  int capacity() const
  {
    static const std::filesystem::path file{cpu_path()/"cpu_capacity"};
    return exists(path) ? std::stoi(read_first_line_to_string(path)) :
      max_capacity();
  }

  /// @returns The (logical) core list.
  std::vector<Range> core_list() const
  {
    const auto path = cpu_path()/"topology/core_cpus_list";
    std::vector<Cpu::Range> result;
    if (exists(path)) {
      const auto line = read_first_line_to_string(path);
      for_each_range([&result](auto&& cpu_range)
      {
        result.push_back(std::move(cpu_range));
        return true;
      }, line);
    }
    return result;
  }

private:
  int index_{-1};

  static const std::filesystem::path& cpu_root_path()
  {
    static std::filesystem::path file{"/sys/devices/system/cpu"};
    return file;
  }

  std::filesystem::path cpu_path() const
  {
    return std::filesystem::path{cpu_root_path()/"cpu"+std::to_string(index_)};
  }

  template<typename F>
  static void for_each_range(F&& callback, const std::string_view line)
  {
    for_each_part([callback = std::forward<F>(callback)](const std::string_view part)mutable
    {
      Cpu::Range range;
      const auto dash_pos = part.find('-');
      if (dash_pos != std::string_view::npos) {
        DMITIGR_ASSERT(dash_pos + 1 < part.size());
        const auto lower = std::stoi(std::string{part.substr(0, dash_pos)});
        const auto upper = std::stoi(std::string{part.substr(dash_pos + 1)});
        range = Cpu::Range{lower, upper};
      } else
        range = Cpu::Range{std::stoi(std::string{part})};
      return std::forward<F>(callback)(std::move(range));
    }, line, str::Fepsep_exact{","});
  }

  static bool is_in_ranges_of(const std::filesystem::path& path, const int idx)
  {
    bool result{};
    for_each_range([&result, idx](const auto& range)
    {
      for (int i{range.lower()}; i <= range.upper(); ++i) {
        if (i == idx) {
          result = true;
          return false;
        }
      }
      return true;
    }, read_first_line_to_string(path));
    return result;
  }
};

/**
 * @brief Calls `callback` for each CPU.
 *
 * @param callback A function of signature `bool(Cpu&&)`.
 */
template<typename F>
void for_each_cpu(F&& callback, const int offset = 0)
{
  for (int i{offset}; ; ++i) {
    if (auto cpu = Cpu::make(i)) {
      if (!std::forward<F>(callback)(std::move(*cpu)))
        break;
    } else
      break;
  }
}

} // namespace dmitigr::nix

#endif  // DMITIGR_NIX_CPU_HPP
