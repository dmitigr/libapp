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

#ifndef DMITIGR_PRG_COMMAND_HPP
#define DMITIGR_PRG_COMMAND_HPP

#include "../base/assert.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace dmitigr::prg {

/**
 * @brief A command.
 *
 * @details Stores the parsed command with it's options and parameters.
 */
class Command final {
public:
  /// The alias to represent a map of command options.
  using Option_map = std::map<std::string, std::optional<std::string>>;

  /// The alias to represent a vector of command parameters.
  using Parameter_vector = std::vector<std::string>;

  /**
   * @brief An option reference.
   *
   * @warning The lifetime of the instances of this class is limited by
   * the lifetime of the corresponding instances of type Command.
   */
  class Optref final {
  public:
    /// @returns `true` if the instance is valid (references an option).
    bool is_valid() const noexcept
    {
      return is_valid_;
    }

    /**
     * @returns `is_valid()`.
     *
     * @par Requires
     * `!value()`.
     */
    bool is_valid_throw_if_value() const
    {
      const auto valid = is_valid();
      if (valid && value_)
        throw_requirement("requires no value");

      return valid;
    }

    /**
     * @returns `is_valid()`.
     *
     * @par Requires
     * `value()`.
     */
    bool is_valid_throw_if_no_value() const
    {
      const auto valid = is_valid();
      if (valid && !value_)
        throw_requirement("requires a value");

      return valid;
    }

    /// @returns `is_valid()`.
    explicit operator bool() const noexcept
    {
      return is_valid();
    }

    /// @returns The corresponding Command instance.
    const Command& command() const noexcept
    {
      return command_;
    }

    /// @returns The name of this option.
    const std::string& name() const
    {
      return name_;
    }

    /**
     * @returns The value of this option.
     *
     * @par Requires
     * `is_valid()`.
     */
    const std::optional<std::string>& value() const
    {
      if (!is_valid())
        throw_requirement("is not valid");
      return value_;
    }

    /**
     * @returns The not null value of this option.
     *
     * @par Requires
     * `value_of_mandatory()`.
     */
    const std::string& value_not_null() const
    {
      const auto& val = value();
      if (!val)
        throw_requirement("requires a value");
      return *val;
    }

    /**
     * @returns The not empty value of this option.
     *
     * @par Requires
     * `!value_not_null().empty()`.
     */
    const std::string& value_not_empty() const
    {
      const auto& val = value_not_null();
      if (val.empty())
        throw_requirement("requires a non empty value");
      return val;
    }

  private:
    friend Command;

    bool is_valid_{};
    const Command& command_;
    std::string name_;
    std::optional<std::string> value_;

    /// The constructor. (Constructs invalid instance.)
    Optref(const Command& command, std::string name) noexcept
      : command_{command}
      , name_{std::move(name)}
    {
      DMITIGR_ASSERT(!is_valid());
    }

    /// The constructor.
    explicit Optref(const Command& command,
      std::string name, std::optional<std::string> value) noexcept
      : is_valid_{true}
      , command_{command}
      , name_{std::move(name)}
      , value_{std::move(value)}
    {
      DMITIGR_ASSERT(is_valid());
    }

    /// @throws `Exception`.
    [[noreturn]] void throw_requirement(const std::string_view requirement) const
    {
      DMITIGR_ASSERT(!requirement.empty());
      throw std::runtime_error{std::string{"option --"}
        .append(name_).append(" ").append(requirement)};
    }
  };

  /// The default constructor.
  Command() = default;

  /**
   * @brief The constructor.
   *
   * @par Requires
   * `!name.empty()`.
   */
  explicit Command(std::string name,
    Option_map options = {}, Parameter_vector parameters = {})
    : name_{std::move(name)}
    , options_{std::move(options)}
    , parameters_{std::move(parameters)}
  {
    if (name_.empty())
      throw std::invalid_argument{"empty command name"};
  }

  /// @returns The command name (or program path).
  const std::string& name() const noexcept
  {
    return name_;
  }

  /// @returns The map of options.
  const Option_map& options() const noexcept
  {
    return options_;
  }

  /// @returns The vector of parameters.
  const Parameter_vector& parameters() const noexcept
  {
    return parameters_;
  }

  /// @returns The option reference, or invalid instance if no option `name`.
  Optref option(const std::string& name) const noexcept
  {
    const auto i = options_.find(name);
    return i != cend(options_) ? Optref{*this, i->first, i->second} :
      Optref{*this, name};
  }

  /// @returns A value of type `std::tuple<Optref, ...>`.
  template<class ... Types>
  auto options(Types&& ... names) const noexcept
  {
    return std::make_tuple(option(std::forward<Types>(names))...);
  }

  /**
   * @returns options(names).
   *
   * @throw Exception if there is an option which doesn't present in `names`.
   */
  template<class ... Types>
  auto options_strict(Types&& ... names) const
  {
    const std::vector<std::string_view> opts{std::forward<Types>(names)...};
    for (const auto& kv : options_)
      if (find(cbegin(opts), cend(opts), kv.first) == cend(opts))
        throw std::runtime_error{std::string{"unexpected option --"}.append(kv.first)};
    return options(std::forward<Types>(names)...);
  }

  /// @returns `option(option_name)`.
  Optref operator[](const std::string& option_name) const noexcept
  {
    return option(option_name);
  }

  /**
   * @returns `parameters()[parameter_index]`.
   *
   * @par Requires
   * `(parameter_index < parameters().size())`.
   */
  const std::string& operator[](const std::size_t parameter_index) const
  {
    if (!(parameter_index < parameters_.size()))
      throw std::invalid_argument{"invalid command parameter index"};
    return parameters_[parameter_index];
  }

private:
  std::string name_;
  Option_map options_;
  Parameter_vector parameters_;
};

/// @returns `true` if `arg` represents a command line option.
inline bool is_option(const std::string_view arg) noexcept
{
  return arg.data() && arg.find("--") == 0;
}

/**
 * @returns The command.
 *
 * @param[in,out] argc_p The pointer to the size of `*argv_p`.
 * @param[in,out] argv_p The pointer to the arguments.
 * @param[in] may_have_params `true` if the command may have parameters.
 *
 * @details Assumed command syntax:
 *   - command [--option[=[value]]] [--] [parameter ...]
 *
 * Each option may have a value specified after the "=" character. The sequence
 * of two dashes ("--") indicates "end of options", so the remaining arguments
 * are treated as parameters.
 *
 * @remarks Short options notation (e.g. `-o` or `-o=1`) doesn't supported
 * and always treated as parameters.
 *
 * @par Requires
 * `(argc_p && *argc_p > 0 && argv_p && *argv_p)` and
 * `((*argv_p)[i] && std::strlen((*argv_p)[0]) > 0)`.
 */
inline Command make_command(int* const argc_p, const char* const** const argv_p,
  const bool may_have_params)
{
  if (!argc_p || !(*argc_p > 0))
    throw std::invalid_argument{"invalid argc"};
  else if (!argv_p || !*argv_p)
    throw std::invalid_argument{"invalid argv"};

  static const auto opt = [](const std::string_view arg)
    -> std::optional<std::pair<std::string, std::optional<std::string>>>
    {
      DMITIGR_ASSERT(arg.data());
      if (is_option(arg)) {
        if (arg.size() == 2) {
          // Empty option (end-of-options marker).
          return std::make_pair(std::string{}, std::nullopt);
        } else if (const auto pos = arg.find('=', 2); pos != std::string::npos) {
          // Option with value.
          auto name = arg.substr(2, pos - 2);
          auto value = arg.substr(pos + 1);
          return std::pair<std::string, std::string>(std::move(name),
            std::move(value));
        } else
          // Option without value.
          return std::make_pair(std::string{arg.substr(2)}, std::nullopt);
      } else
        // Not an option.
        return std::nullopt;
    };

  static const auto check_argv = [](const int argi, const char* const* argv)
  {
    if (!argv[argi])
      throw std::invalid_argument{std::string{"invalid argv["}
        .append(std::to_string(argi)).append("]")};
  };

  int argi{};
  const int argc{*argc_p};
  const char* const* argv{*argv_p};
  {
    check_argv(argi, argv);

    std::string name{argv[argi]};
    if (name.empty())
      throw std::invalid_argument{std::string{"empty argv["}
        .append(std::to_string(argi)).append("]")};

    // Declare command data.
    Command::Option_map options;
    Command::Parameter_vector parameters;

    // Increment argument index.
    ++argi;

    // Collect options.
    for (; argi < argc; ++argi) {
      check_argv(argi, argv);
      if (auto o = opt(argv[argi])) {
        if (o->first.empty()) {
          // End-of-options detected.
          ++argi;
          break;
        } else
          options[std::move(o->first)] = std::move(o->second);
      } else
        // A parameter detected.
        break;
    }

    // Collect parameters if the command may have ones.
    if (may_have_params) {
      for (; argi < argc; ++argi) {
        check_argv(argi, argv);
        if (is_option(argv[argi]))
          throw std::runtime_error{"options must precede the parameters"};
        parameters.emplace_back(argv[argi]);
      }
    }

    // Modify output arguments.
    *argc_p -= argi;
    *argv_p += argi;

    // Collect result.
    return Command{std::move(name), std::move(options), std::move(parameters)};
  }
}

} // namespace dmitigr::prg

#endif // DMITIGR_PRG_COMMAND_HPP
