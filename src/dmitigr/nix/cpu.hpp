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

#if !defined(__linux__) && !defined(__APPLE__)
#error dmitigr/nix/cpu.hpp is usable only on Linux or macOS!
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

/// @returns `true` if Simultaneous Multithreading is available.
inline bool is_smt_available()
{
#ifdef __linux__
  if (std::ifstream list{"/sys/devices/system/cpu/cpu0/topology/core_cpus_list"}) {
    const auto line = read_line_to_string(list);
    return line.find_first_of(",-") != std::string::npos;
  }
#endif
  return false;
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
    : Cpu{index, int{}}
  {
#ifdef __linux__
    DMITIGR_CKARG(exists(system_path()));
#endif
  }

  /// @returns An instance of CPU at index `index` or `std::nullopt`.
  static std::optional<Cpu> make(const int index)
  {
#ifdef __linux__
    Cpu result{index, int{}};
    if (exists(result.system_path()))
      return std::optional{std::move(result)};
#endif
    return std::nullopt;
  }

  /// @returns `true` if this instance is valid.
  bool is_valid() const noexcept
  {
    return index() >= 0;
  }

  /// @returns `true` if this CPU core is physical.
  bool is_physical() const noexcept
  {
    const auto list = core_list();
    return list.empty() || list.front().lower() == index();
  }

  /// @returns `true` if this CPU core is performant (P-core).
  bool is_performant() const noexcept
  {
    static const std::filesystem::path cpus{"/sys/devices/cpu_core/cpus"};
    if (exists(cpus)) {
      bool result{};
      for_each_range([&result, this](const auto& range)
      {
        for (int i{range.lower()}; i <= range.upper(); ++i) {
          if (i == index()) {
            result = true;
            return false;
          }
        }
        return true;
      }, read_first_line_to_string(cpus));
      return result;
    }
    return true;
  }

  /// @returns An index of this CPU.
  int index() const noexcept
  {
    return index_;
  }

#ifdef __linux__

  /// @returns Max CPU capacity.
  static constexpr int max_capacity() noexcept
  {
    return 1024;
  }

  /// @returns CPU capacity.
  int capacity() const
  {
    if (const auto path = system_path()/"cpu_capacity"; exists(path))
      return std::stoi(read_first_line_to_string(path));
    return max_capacity();
  }

#endif

  /// @returns The (logical) core list.
  std::vector<Range> core_list() const
  {
    const auto path = system_path()/"topology/core_cpus_list";
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
  std::vector<Range> core_list_;

  Cpu(const int index, int)
    : index_{index}
  {}

  std::filesystem::path system_path() const
  {
    return std::filesystem::path{"/sys/devices/system/cpu/cpu"+std::to_string(index_)};
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
