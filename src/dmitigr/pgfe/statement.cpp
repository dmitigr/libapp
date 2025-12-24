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

#include "../base/assert.hpp"
#include "../base/str.hpp"
#include "../base/traits.hpp"
#include "connection.hpp"
#include "statement.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <iterator>
#include <memory>

namespace dmitigr::pgfe {

namespace {

inline bool is_ident_char(const auto ch) noexcept
{
  const auto c = static_cast<unsigned char>(ch);
  return std::isalnum(c) || c == '_' || c == '$';
}

inline bool is_named_param_char(const auto ch) noexcept
{
  const auto c = static_cast<unsigned char>(ch);
  return std::isalnum(c) || c == '_' || c == '-';
}

inline bool is_quote_char(const auto ch) noexcept
{
  const auto c = static_cast<unsigned char>(ch);
  return c == '\'' || c == '\"';
}

inline void append_str(char** result, const std::string& str) noexcept
{
  std::memcpy(*result, str.data(), str.size());
  *result += str.size();
}

inline void append_lit(char** result, const auto& lit) noexcept
{
  std::memcpy(*result, lit, str::len(lit));
  *result += str::len(lit);
}

inline void append_chr(char** result, const char ch) noexcept
{
  **result = ch;
  ++(*result);
}

} // namespace

// =============================================================================
// Statement::Fragment
// =============================================================================

DMITIGR_PGFE_INLINE
Statement::Fragment::Fragment(const Type tp, const int dp, const std::string& s)
  : type{tp}
  , depth{dp}
  , str{s}
{}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_text() const noexcept
{
  return type == Fragment::Type::text;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_quoted_text() const noexcept
{
  return type == Fragment::Type::quoted_text;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_unquoted_named_parameter() const noexcept
{
  return type == Fragment::Type::named_parameter_unquoted;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_unquoted_named_parameter(const std::string_view name) const noexcept
{
  return is_unquoted_named_parameter() && str == name;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_quoted_named_parameter() const noexcept
{
  using enum Fragment::Type;
  return type == named_parameter_literal || type == named_parameter_identifier;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_quoted_named_parameter(const std::string_view name) const noexcept
{
  return is_quoted_named_parameter() && str == name;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_named_parameter() const noexcept
{
  return is_unquoted_named_parameter() || is_quoted_named_parameter();
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_named_parameter(const std::string_view name) const noexcept
{
  return is_named_parameter() && str == name;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_positional_parameter() const noexcept
{
  return type == Fragment::Type::positional_parameter;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_parameter() const noexcept
{
  return is_named_parameter() || is_positional_parameter();
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_one_line_comment() const noexcept
{
  return type == Fragment::Type::one_line_comment;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_multi_line_comment() const noexcept
{
  return type == Fragment::Type::multi_line_comment;
}

DMITIGR_PGFE_INLINE bool
Statement::Fragment::is_comment() const noexcept
{
  return is_one_line_comment() || is_multi_line_comment();
}

DMITIGR_PGFE_INLINE
const std::string& Statement::Fragment::norm_str() const
{
  if (type == Type::text) {
    if (norm.empty()) {
      const auto str_size = str.size();
      norm.reserve(2 * str_size);
      enum { word, spec, space } prev_char_type{space}, prev_non_space{spec};
      for (std::string::size_type i{}; i < str_size; ++i) {
        const auto ch = static_cast<unsigned char>(str[i]);
        if (is_ident_char(ch)) {
          if (prev_char_type == space && prev_non_space == word)
            norm += ' ';
          norm += static_cast<char>(std::tolower(ch));
          prev_char_type = prev_non_space = word;
        } else if (!std::isspace(ch)) {
          norm += static_cast<char>(ch);
          prev_char_type = prev_non_space = spec;
        } else
          prev_char_type = space;
      }
      if (!norm.empty() && norm.back() == ' ')
        norm.pop_back();
    }
    return norm;
  }
  return str;
}

DMITIGR_PGFE_INLINE
bool Statement::Fragment::norm_equal(const Fragment& rhs) const
{
  return type == rhs.type && depth == rhs.depth && norm_str() == rhs.norm_str();
}

DMITIGR_PGFE_INLINE void Statement::Fragment::renormalize()
{
  norm.clear();
  norm_str();
}

// =============================================================================
// Statement::Comments
// =============================================================================

/// Represents an API for comments processing.
struct Statement::Comments final {
public:
  /// Denotes the key type of the associated data.
  using Key = Statement::Metadata::Key;

  /// Denotes the value type of the associated data.
  using Value = Statement::Metadata::Value;

  /// Denotes the fragment type.
  using Fragment = Statement::Fragment;

  /// Denotes the fragment list type.
  using Fragment_list = Statement::Fragment_list;

  /// @returns The vector of associated metadata.
  static std::vector<std::pair<Key, Value>>
  metadata(const Fragment_list& fragments)
  {
    const auto [begin, end] = related(fragments);
    if (begin != cend(fragments)) {
      const auto comments = joined(begin, end);
      return metadata(comments);
    }
    return {};
  }

  /**
   * @brief Finds relevant comments in the specified fragments.
   *
   * @returns The pair of iterators that specifies a range of relevant comments,
   * or pair of cend(fragments) if no relevant comments found.
   */
  std::pair<Fragment_list::const_iterator, Fragment_list::const_iterator>
  static related(const Fragment_list& fragments)
  {
    const auto b = cbegin(fragments);
    const auto e = cend(fragments);
    auto result = std::make_pair(e, e);

    enum class Scan_front {
      many_new_lines,
      all_spaces_no_new_line,
      all_spaces_one_new_line,
      front_spaces_no_new_line,
      front_spaces_one_new_line
    };

    static const auto scan_front = [](const std::string_view str) noexcept
    {
      using enum Scan_front;
      int count{};
      for (const auto c : str) {
        if (c == '\n') {
          ++count;
          if (count > 1)
            return many_new_lines;
        } else if (!str::is_space(static_cast<unsigned char>(c)))
          return !count ? front_spaces_no_new_line : front_spaces_one_new_line;
      }
      return !count ? all_spaces_no_new_line : all_spaces_one_new_line;
    };

    // Try to find the first fragment which is the "nearby string".
    const auto first_nearby = find_if(b, e, [](const Fragment& f) noexcept
    {
      return !f.is_comment() && (!f.is_text() || [&f]
      {
        using enum Scan_front;
        const auto r = scan_front(f.str);
        return r == front_spaces_no_new_line || r == front_spaces_one_new_line;
      }());
    });
    if (first_nearby != b) {
      auto i = first_nearby;
      result.second = first_nearby;
      do {
        --i;
        if (i->is_text()) {
          if (const auto p = prev(i); p->is_multi_line_comment()) {
            DMITIGR_ASSERT(!i->str.empty());
            using enum Scan_front;
            const auto sfr = scan_front(i->str);
            if (sfr == all_spaces_no_new_line || sfr == all_spaces_one_new_line)
              i = p;
            else
              break;
          } else
            break;
        } else if (!i->is_comment())
          break;
        result.first = i;
      } while (i != b);
    }
    DMITIGR_ASSERT(result.first == e || result.first->is_comment());
    return result;
  }

  /// @returns The string with joined comments.
  static std::string joined(Fragment_list::const_iterator i,
    const Fragment_list::const_iterator e)
  {
    std::string result;
    while (i != e) {
      using enum Fragment::Type;
      switch (i->type) {
      case one_line_comment:
        do {
          result.append(i->str).append(1, '\n');
          ++i;
        } while (i != e && i->type == one_line_comment);
        result.pop_back();
        continue;
      case multi_line_comment:
        result.append(i->str);
        break;
      default:
        break;
      }
      ++i;
    }
    return result;
  }

private:
  /**
   * @brief Extracts the metadata from dollar quoted literals of `input`.
   *
   * @returns The vector of metadata.
   */
  static std::vector<std::pair<Key, Value>>
  metadata(const std::string_view input)
  {
    static const auto is_valid_tag_char = [](const auto ch) noexcept
    {
      const auto c = static_cast<unsigned char>(ch);
      return std::isalnum(c) || c == '_' || c == '-';
    };

    enum { top, dollar, dollar_quote_leading_tag,
      dollar_quote, dollar_quote_dollar } state = top;
    std::vector<std::pair<Key, Value>> result;
    std::string value;
    std::string dollar_quote_leading_tag_name;
    std::string dollar_quote_trailing_tag_name;
    std::string::size_type border{};
    for (const auto current_char : input) {
    start:
      switch (state) {
      case top:
        if (current_char == '$')
          state = dollar;
        else if (current_char == '\n')
          border = 0;
        else
          ++border;
        continue;
      case dollar:
        if (is_valid_tag_char(current_char)) {
          state = dollar_quote_leading_tag;
          dollar_quote_leading_tag_name += current_char;
        } else
          state = top;
        continue;
      case dollar_quote_leading_tag:
        if (current_char == '$')
          state = dollar_quote;
        else if (is_valid_tag_char(current_char))
          dollar_quote_leading_tag_name += current_char;
        else {
          dollar_quote_leading_tag_name.clear();
          state = top;
          goto start;
        }
        continue;
      case dollar_quote:
        if (current_char == '$')
          state = dollar_quote_dollar;
        value += current_char;
        continue;
      case dollar_quote_dollar:
        if (current_char == '$') {
          if (dollar_quote_leading_tag_name == dollar_quote_trailing_tag_name) {
            /*
             * Okay, the key and value are successfully extracted.
             * Now attempt to clean up the value before adding it to the result.
             */
            DMITIGR_ASSERT(value.back() == '$');
            value.pop_back();
            state = top;
            result.emplace_back(dollar_quote_leading_tag_name,
              cleaned_value(value, border));
            value.clear();
            dollar_quote_leading_tag_name.clear();
            dollar_quote_trailing_tag_name.clear();
          } else {
            state = dollar_quote;
            value += dollar_quote_trailing_tag_name;
            dollar_quote_trailing_tag_name.clear();
            goto start;
          }
        } else
          dollar_quote_trailing_tag_name += current_char;
        continue;
      }
    }
    return result;
  }

  /**
   * @brief Cleans up the metadata value.
   *
   * #details Cleaning up includes:
   *   -# removing the out of border characters;
   *   -# trimming the most leading and the most trailing newline characters.
   */
  static std::string cleaned_value(std::string_view value,
    const std::string::size_type border)
  {
    // Skip the most leading newline character.
    const auto start = [value]
    {
      std::string_view::size_type pos{};
      const auto value_size = value.size();
      if (pos < value_size && value[pos] == '\r')
        ++pos;
      if (pos < value_size && value[pos] == '\n')
        ++pos;
      if (pos && value[pos - 1] != '\n')
        pos = 0;
      return pos;
    }();
    value = value.substr(start);

    std::string result;
    if (border) {
      std::string_view::size_type count{start ? border : 0u};
      enum { skiping, storing } state = count ? skiping : storing;
      for (const auto current_char : value) {
        switch (state) {
        case skiping:
          if (count > 1)
            --count;
          else
            state = storing;
          continue;
        case storing:
          if (current_char == '\n') {
            count = border;
            state = skiping;
          }
          result += current_char;
          continue;
        }
      }
    } else
      result = std::string{value};

    // Trim the most trailing newline character.
    if (std::string::size_type size{result.size()}) {
      if (start < size && result[size - 1] == '\n') {
        --size;
        if (start < size && result[size - 1] == '\r')
          --size;
      }
      result.resize(size);
    }

    return result;
  }
};

// =============================================================================
// Statement
// =============================================================================

DMITIGR_PGFE_INLINE Statement::Statement(const std::string_view text)
  : Statement{parse_sql_input(text).first}
{
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Statement::Statement(const std::string& text)
  : Statement{std::string_view{text}}
{}

DMITIGR_PGFE_INLINE Statement::Statement(const char* const text)
  : Statement{std::string_view{text}}
{}

DMITIGR_PGFE_INLINE std::size_t
Statement::positional_parameter_count() const noexcept
{
  return positional_parameters_.size();
}

DMITIGR_PGFE_INLINE std::size_t Statement::named_parameter_count() const noexcept
{
  return named_parameters_.size();
}

DMITIGR_PGFE_INLINE std::size_t Statement::parameter_count() const noexcept
{
  return positional_parameter_count() + named_parameter_count();
}

DMITIGR_PGFE_INLINE bool Statement::has_positional_parameter() const noexcept
{
  return !positional_parameters_.empty();
}

DMITIGR_PGFE_INLINE bool Statement::has_named_parameter() const noexcept
{
  return !named_parameters_.empty();
}

DMITIGR_PGFE_INLINE bool Statement::has_parameter() const noexcept
{
  return has_positional_parameter() || has_named_parameter();
}

DMITIGR_PGFE_INLINE bool
Statement::has_parameter(const std::string_view name) const noexcept
{
  return parameter_index(name) < parameter_count();
}

DMITIGR_PGFE_INLINE std::string_view
Statement::parameter_name(const std::size_t index) const
{
  if (!((positional_parameter_count() <= index) && (index < parameter_count())))
    throw Generic_exception{"cannot get Statement parameter name"};
  return named_parameters_[index - positional_parameter_count()].name;
}

DMITIGR_PGFE_INLINE std::size_t
Statement::parameter_index(const std::string_view name) const noexcept
{
  return named_parameter_index(name);
}

DMITIGR_PGFE_INLINE bool Statement::is_empty() const noexcept
{
  return fragments_.empty();
}

DMITIGR_PGFE_INLINE bool Statement::is_query_empty() const noexcept
{
  return all_of(cbegin(fragments_), cend(fragments_),
    [](const Fragment& f) noexcept
    {
      return f.is_comment() || (f.is_text() && str::is_all_spaces(f.str));
    });
}

DMITIGR_PGFE_INLINE bool
Statement::is_parameter_missing(const std::size_t index) const
{
  if (!(index < positional_parameter_count()))
    throw Generic_exception{"cannot determine if Statement parameter is missing"};
  return positional_parameters_[index] <= 0;
}

DMITIGR_PGFE_INLINE bool
Statement::is_parameter_literal(const std::size_t index) const
{
  if (!((positional_parameter_count() <= index) && (index < parameter_count())))
    throw Generic_exception{"cannot determine if Statement parameter is literal"};
  return named_parameter_type(index) == Fragment::Type::named_parameter_literal;
}

DMITIGR_PGFE_INLINE bool
Statement::is_parameter_literal(const std::string_view name) const
{
  return is_parameter_literal(parameter_index(name));
}

DMITIGR_PGFE_INLINE bool
Statement::is_parameter_identifier(const std::size_t index) const
{
  if (!((positional_parameter_count() <= index) && (index < parameter_count())))
    throw Generic_exception{"cannot determine if Statement parameter is identifier"};
  return named_parameter_type(index) == Fragment::Type::named_parameter_identifier;
}

DMITIGR_PGFE_INLINE bool
Statement::is_parameter_identifier(const std::string_view name) const
{
  return is_parameter_identifier(parameter_index(name));
}

DMITIGR_PGFE_INLINE bool Statement::has_missing_parameter() const noexcept
{
  return any_of(cbegin(positional_parameters_), cend(positional_parameters_),
    [](const auto count) noexcept { return count <= 0; });
}

DMITIGR_PGFE_INLINE bool
Statement::has_duplicate_positional_parameter() const noexcept
{
  return any_of(cbegin(positional_parameters_), cend(positional_parameters_),
    [](const auto count) noexcept
    {
      return count > 1;
    });
}

DMITIGR_PGFE_INLINE bool
Statement::has_duplicate_named_parameter() const noexcept
{
  return any_of(cbegin(named_parameters_), cend(named_parameters_),
    [](const auto& param) noexcept
    {
      return param.count > 1;
    });
}

DMITIGR_PGFE_INLINE bool
Statement::has_literal_parameter() const noexcept
{
  return any_of(cbegin(named_parameters_), cend(named_parameters_),
    [](const auto& param) noexcept
    {
      return param.type == Fragment::Type::named_parameter_literal;
    });
}

DMITIGR_PGFE_INLINE bool
Statement::has_identifier_parameter() const noexcept
{
  return any_of(cbegin(named_parameters_), cend(named_parameters_),
    [](const auto& param) noexcept
    {
      return param.type == Fragment::Type::named_parameter_identifier;
    });
}

DMITIGR_PGFE_INLINE bool
Statement::has_quoting_parameter() const noexcept
{
  return any_of(cbegin(named_parameters_), cend(named_parameters_),
    [](const auto& param) noexcept
    {
      using enum Fragment::Type;
      return param.type == named_parameter_literal ||
        param.type == named_parameter_identifier;
    });
}

DMITIGR_PGFE_INLINE void Statement::append(const Statement& appendix)
{
  fragments_.insert(cend(fragments_), cbegin(appendix.fragments_),
    cend(appendix.fragments_));
  update_cache(appendix); // can throw
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE Statement&
Statement::bind(const std::string& name, std::string value)
{
  if (!has_parameter(name))
    throw Generic_exception{"cannot bind Statement parameter"};

  bindings_[name] = std::move(value);

  assert(is_invariant_ok());
  return *this;
}

DMITIGR_PGFE_INLINE Statement&
Statement::unbind(const std::string& name)
{
  if (!has_parameter(name))
    throw Generic_exception{"cannot unbind Statement parameter"};

  if (const auto i = bindings_.find(name); i != bindings_.cend())
    bindings_.erase(i);

  assert(is_invariant_ok());
  return *this;
}

DMITIGR_PGFE_INLINE const std::string*
Statement::bound(const std::string& name) const noexcept
{
  const auto i = bindings_.find(name);
  return i != bindings_.cend() ? std::addressof(i->second) : nullptr;
}

DMITIGR_PGFE_INLINE std::size_t
Statement::bound_parameter_count() const noexcept
{
  return bindings_.size();
}

DMITIGR_PGFE_INLINE bool Statement::has_bound_parameter() const noexcept
{
  return !bindings_.empty();
}

DMITIGR_PGFE_INLINE bool Statement::has_unbound_parameter() const noexcept
{
  return bound_parameter_count() < named_parameter_count();
}

DMITIGR_PGFE_INLINE void
Statement::replace(const std::string_view name, const Statement& replacement)
{
  if (!(has_parameter(name) && (std::addressof(replacement) != this)))
    throw Generic_exception{"cannot replace Statement parameter " +
      std::string{name}};

  const auto normalized = [this, &replacement]
  {
    if (is_normalized() || replacement.is_normalized()) {
      normalize();
      replacement.normalize();
      return true;
    }
    return false;
  }();

  const auto update_fragments = [name, normalized](auto& fragments,
    const auto& replacement_fragments)
  {
    const auto renormalize = [normalized](const auto& iter)
    {
      if (normalized)
        iter->renormalize();
    };

    for (auto fi = begin(fragments); fi != end(fragments);) {
      if (fi->is_named_parameter(name)) {
        // Insert the `replacement` just before `fi`.
        auto first = fragments.insert(fi, cbegin(replacement_fragments),
          cend(replacement_fragments));

        // Update the depth of inserted fragments.
        const auto fi_depth = fi->depth;
        for (auto i = first; i != fi; ++i)
          i->depth += fi_depth;

        // Erase named parameter pointed by `fi` and get the next iterator.
        fi = fragments.erase(fi);

        /*
         * Join first and last text fragments of the replacement with the
         * fragments bordering them.
         */
        if (auto rsz = replacement_fragments.size()) {
          if (first != begin(fragments) && first->is_text()) {
            const auto prefirst = prev(first);
            if (prefirst->is_text() && first->depth == prefirst->depth) {
              prefirst->str.append(first->str);
              renormalize(prefirst);
              first = fragments.erase(first);
              --rsz;
            }
          }

          if (fi != end(fragments) && fi->is_text()) {
            const auto last = rsz ? prev(fi) : first;
            if (last->is_text() && last->depth == fi->depth) {
              last->str.append(fi->str);
              renormalize(last);
              fi = fragments.erase(fi);
            }
          }
        }
      } else
        ++fi;
    }
  };

  update_fragments(fragments_, replacement.fragments_); // can throw
  update_fragments(norm_fragments_, replacement.norm_fragments_); // can throw
  update_cache(replacement);  // can throw

  assert(is_invariant_ok());
}

// private
template<bool IsQueryString>
std::string::size_type
Statement::string_capacity() const noexcept(!IsQueryString)
{
  std::string::size_type result{};
  for (const auto& fragment : fragments_) {
    using enum Fragment::Type;
    switch (fragment.type) {
    case text:
      [[fallthrough]];
    case quoted_text:
      result += fragment.str.size();
      break;
    case one_line_comment:
      result += str::len("--");
      result += fragment.str.size();
      result += str::len("\n");
      break;
    case multi_line_comment:
      result += str::len("/*");
      result += fragment.str.size();
      result += str::len("*/");
      break;
    case named_parameter_unquoted:
      if constexpr (IsQueryString) {
        if (const auto* const value = bound(fragment.str)) {
          result += value->size();
          break;
        }
        result += str::len("$") + str::len("65535");
      } else {
        result += str::len(":");
        result += str::len("{");
        result += fragment.str.size();
        result += str::len("}");
      }
      break;
    case named_parameter_literal:
      if constexpr (IsQueryString) {
        result += 2 * str::len("'");
        if (const auto* const value = bound(fragment.str))
          result += 2 * value->size();
        else
          result += str::len("NULL");
      } else {
        result += str::len(":");
        result += str::len("'");
        result += fragment.str.size();
        result += str::len("'") ;
      }
      break;
    case named_parameter_identifier:
      if constexpr (IsQueryString) {
        result += 2 * str::len("\"");
        if (const auto* const value = bound(fragment.str))
          result += 2 * value->size();
        else
          throw Generic_exception{"cannot calculate query string capacity: "
            "named parameter "+fragment.str+" declared as identifier has no "
            "value bound"};
      } else {
        result += str::len(":");
        result += str::len("\"");
        result += fragment.str.size();
        result += str::len("\"");
      }
      break;
    case positional_parameter:
      result += str::len("$");
      result += fragment.str.size();
      break;
    }
  }
  return result;
}

// private
template<bool IsQueryString>
std::string::size_type
Statement::write_string(char* result, const Connection* const connection) const
{
  using enum Fragment::Type;

  if constexpr (IsQueryString) {
    if (has_missing_parameter())
      throw Generic_exception{"cannot write query string: "
        "Statement has missing parameter"};
  } else
    DMITIGR_ASSERT(!connection);

  static const auto check_quoted_named_parameter = [](const auto& fragment,
    const auto* const conn, const auto* const value)
  {
    DMITIGR_ASSERT(fragment.is_quoted_named_parameter());
    const bool is_conn_ok{conn && conn->is_connected()};
    if (is_conn_ok && value)
      return;

    const char* const type_str =
      fragment.type == named_parameter_literal ? "literal" :
      fragment.type == named_parameter_identifier ? "identifier" : nullptr;
    DMITIGR_ASSERT(type_str);
    std::string what{"named parameter "};
    what.append(fragment.str).append(" declared as ").append(type_str);
    if (!is_conn_ok)
      what.append(" cannot be quoted without the active Connection object");
    if (!value) {
      if (!is_conn_ok)
        what.append(" and");
      what.append(" value bound");
    }
    throw Generic_exception{what};
  };

  const char* const begin{result};
  std::size_t bound_counter{};
  if constexpr (!IsQueryString)
    (void)bound_counter;
  for (const auto& fragment : fragments_) {
    switch (fragment.type) {
    case text:
      [[fallthrough]];
    case quoted_text:
      append_str(&result, fragment.str);
      break;
    case one_line_comment:
      append_lit(&result, "--");
      append_str(&result, fragment.str);
      append_chr(&result, '\n');
      break;
    case multi_line_comment:
      append_lit(&result, "/*");
      append_str(&result, fragment.str);
      append_lit(&result, "*/");
      break;
    case named_parameter_unquoted:
      if constexpr (IsQueryString) {
        if (const auto* const value = bound(fragment.str); !value) {
          const auto idx = named_parameter_index(fragment.str);
          DMITIGR_ASSERT(idx >= positional_parameter_count());
          DMITIGR_ASSERT(idx < parameter_count());
          append_chr(&result, '$');
          append_str(&result, std::to_string(idx - bound_counter + 1));
        } else {
          append_str(&result, *value);
          ++bound_counter;
        }
      } else {
        append_chr(&result, ':');
        append_chr(&result, '{');
        append_str(&result, fragment.str);
        append_chr(&result, '}');
      }
      break;
    case named_parameter_literal:
      if constexpr (IsQueryString) {
        if (const auto* const value = bound(fragment.str))
          append_str(&result, connection->to_quoted_literal(*value));
        else
          append_lit(&result, "NULL");
      } else {
        append_chr(&result, ':');
        append_chr(&result, '\'');
        append_str(&result, fragment.str);
        append_chr(&result, '\'');
      }
      break;
    case named_parameter_identifier: {
      if constexpr (IsQueryString) {
        const auto* const value = bound(fragment.str);
        check_quoted_named_parameter(fragment, connection, value);
        append_str(&result, connection->to_quoted_identifier(*value));
      } else {
        append_chr(&result, ':');
        append_chr(&result, '"');
        append_str(&result, fragment.str);
        append_chr(&result, '"');
      }
      break;
    }
    case positional_parameter:
      append_chr(&result, '$');
      append_str(&result, fragment.str);
      break;
    }
  }
  if constexpr (IsQueryString)
    DMITIGR_ASSERT(bound_counter <= bound_parameter_count());
  if (!fragments_.empty() && fragments_.back().is_one_line_comment()) {
    DMITIGR_ASSERT(*result == '\n');
    --result;
  }
  return static_cast<std::string::size_type>(result - begin);
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::string_capacity() const noexcept
{
  return string_capacity<false>();
}

DMITIGR_PGFE_INLINE void Statement::erase(const Part part)
{
  const auto related_comments = [this, part]
  {
    const auto e = fragments_.cend();
    return bool(part & Part::comments) ?
      Comments::related(fragments_) : std::pair{e, e};
  };

  const auto erase_not_related_comments = [this](auto i, const auto& end)
  {
    while (i != end) {
      if (i->is_comment())
        i = fragments_.erase(i);
      else
        ++i;
    }
  };

  const auto erase_edge_spaces = [this](const auto i, const auto& end)
  {
    if (i != end && i->is_text()) {
      constexpr bool is_reverse{Is_reverse_iterator_v<std::decay_t<decltype(i)>>};
      constexpr auto side = is_reverse ? str::Trim::rhs : str::Trim::lhs;
      str::trim_spaces(i->str, side);
      if (i->str.empty()) {
        if constexpr (is_reverse)
          fragments_.erase(prev(i.base()));
        else
          fragments_.erase(i);
      } else
        i->renormalize();
    }
  };

  const auto [related_b, related_e] = related_comments();

  if (bool(part & Part::unrelated_comments))
    erase_not_related_comments(fragments_.begin(), related_b);

  if (bool(part & Part::inner_comments))
    erase_not_related_comments(related_e, fragments_.cend());

  if (bool(part & Part::related_comments))
    fragments_.erase(related_b, related_e);

  if (bool(part & Part::leading_spaces))
    erase_edge_spaces(fragments_.begin(), fragments_.cend());

  if (bool(part & Part::trailing_spaces))
    erase_edge_spaces(fragments_.rbegin(), fragments_.crend());
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::write_string(char* result) const
{
  return write_string<false>(result, nullptr);
}

DMITIGR_PGFE_INLINE std::string Statement::to_string() const
{
  std::string result(string_capacity(), '\0');
  const auto size = write_string(result.data());
  DMITIGR_ASSERT(size <= result.capacity());
  result.resize(size);
  return result;
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::query_string_capacity() const
{
  return string_capacity<true>();
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::write_query_string(char* result, const Connection* const connection) const
{
  return write_string<true>(result, connection);
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::write_query_string(char* const result, const Connection& conn) const
{
  return write_query_string(result, std::addressof(conn));
}

DMITIGR_PGFE_INLINE std::string::size_type
Statement::write_query_string(char* const result) const
{
  return write_query_string(result, nullptr);
}

DMITIGR_PGFE_INLINE std::string
Statement::to_query_string(const Connection* const conn) const
{
  std::string result(query_string_capacity(), '\0');
  const auto size = write_query_string(result.data(), conn);
  DMITIGR_ASSERT(size <= result.capacity());
  result.resize(size);
  return result;
}

DMITIGR_PGFE_INLINE std::string
Statement::to_query_string(const Connection& conn) const
{
  return to_query_string(std::addressof(conn));
}

DMITIGR_PGFE_INLINE std::string Statement::to_query_string() const
{
  return to_query_string(nullptr);
}

DMITIGR_PGFE_INLINE auto Statement::metadata() const -> const Metadata&
{
  if (!metadata_)
    metadata_.emplace(Comments::metadata(fragments_));
  return *metadata_;
}

DMITIGR_PGFE_INLINE bool Statement::is_equivalent(const Statement& rhs) const
{
  normalize();
  rhs.normalize();
  const auto& frags = norm_fragments_;
  const auto& rfrags = rhs.norm_fragments_;
  return equal(frags.cbegin(), frags.cend(), rfrags.cbegin(), rfrags.cend(),
    [](const auto& lh, const auto& rh) noexcept
    {
      return lh.is_named_parameter() && rh.is_named_parameter() ?
        lh.type == rh.type : lh.norm_equal(rh);
    });
}

DMITIGR_PGFE_INLINE bool
Statement::are_named_parameters_equal(const Statement& rhs) const
{
  return named_parameters_ == rhs.named_parameters_;
}

DMITIGR_PGFE_INLINE bool Statement::is_equal(const Statement& rhs) const
{
  return is_equivalent(rhs) && are_named_parameters_equal(rhs);
}

DMITIGR_PGFE_INLINE void Statement::normalize() const
{
  if (is_normalized())
    return;

  Fragment_list norm_fragments;
  for (const auto& fragment : fragments_) {
    if (fragment.is_text()) {
      if (!str::is_all_spaces(fragment.str)) {
        if (!norm_fragments.empty() &&
          norm_fragments.back().is_text() &&
          fragment.depth == norm_fragments.back().depth)
          norm_fragments.back().str.append(fragment.str);
        else
          norm_fragments.push_back(fragment);
      }
    } else if (!fragment.is_comment())
      norm_fragments.push_back(fragment);
  }

  for (const auto& fragment : norm_fragments)
    fragment.norm_str();

  norm_fragments_.swap(norm_fragments);
}

DMITIGR_PGFE_INLINE bool Statement::is_normalized() const noexcept
{
  return !norm_fragments_.empty();
}

DMITIGR_PGFE_INLINE bool Statement::is_invariant_ok() const noexcept
{
  const bool positional_parameters_ok =
    (positional_parameter_count() > 0) == has_positional_parameter();
  const bool named_parameters_ok =
    (named_parameter_count() > 0) == has_named_parameter();
  const bool parameters_ok =
    (parameter_count() > 0) == has_parameter();
  const bool parameters_count_ok =
    parameter_count() == (positional_parameter_count() + named_parameter_count());
  const bool bindings_ok = bindings_.size() <= named_parameter_count();
  const bool empty_ok = !is_empty() || !has_parameter();
  const bool parameterizable_ok = Parameterizable::is_invariant_ok();

  return
    positional_parameters_ok &&
    named_parameters_ok &&
    parameters_ok &&
    parameters_count_ok &&
    bindings_ok &&
    empty_ok &&
    parameterizable_ok;
}

// ---------------------------------------------------------------------------
// Statement initializers
// ---------------------------------------------------------------------------

DMITIGR_PGFE_INLINE void
Statement::push_back_fragment(const Fragment::Type type,
  const int depth, const std::string& str)
{
  fragments_.emplace_back(type, depth, str);
  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE void
Statement::push_text(const int depth, const std::string& str)
{
  if (!str.empty())
    push_back_fragment(Fragment::Type::text, depth, str);
}

DMITIGR_PGFE_INLINE void
Statement::push_quoted_text(const int depth, const std::string& str)
{
  push_back_fragment(Fragment::Type::quoted_text, depth, str);
}

DMITIGR_PGFE_INLINE void
Statement::push_one_line_comment(const int depth, const std::string& str)
{
  push_back_fragment(Fragment::Type::one_line_comment, depth, str);
}

DMITIGR_PGFE_INLINE void
Statement::push_multi_line_comment(const int depth, const std::string& str)
{
  push_back_fragment(Fragment::Type::multi_line_comment, depth, str);
}

DMITIGR_PGFE_INLINE void
Statement::push_positional_parameter(const int depth, const std::string& str)
{
  DMITIGR_ASSERT(!str.empty());

  push_back_fragment(Fragment::Type::positional_parameter, depth, str);

  using Size = std::vector<bool>::size_type;
  const int position = stoi(str);
  if (position < 1 || static_cast<Size>(position) > max_parameter_count())
    throw Generic_exception{"invalid parameter position \"" + str + "\""};
  else if (static_cast<Size>(position) > positional_parameters_.size())
    positional_parameters_.resize(static_cast<Size>(position), 0);

  // Increase parameter count.
  positional_parameters_[static_cast<Size>(position) - 1] += 1;

  assert(is_invariant_ok());
}

DMITIGR_PGFE_INLINE void
Statement::push_named_parameter(const int depth, const std::string& str,
  const char quote_char)
{
  DMITIGR_ASSERT(!str.empty());
  DMITIGR_ASSERT(!quote_char || is_quote_char(quote_char));

  if (parameter_count() < max_parameter_count()) {
    using enum Fragment::Type;
    const auto type =
      quote_char == '\'' ? named_parameter_literal :
      quote_char == '\"' ? named_parameter_identifier : named_parameter_unquoted;
    push_back_fragment(type, depth, str);
    const auto e = end(named_parameters_);
    const auto p = find_if(begin(named_parameters_), e,
      [&str](const auto& param) noexcept
      {
        return param.name == str;
      });
    if (p == e)
      named_parameters_.emplace_back(type, str);
    else
      ++p->count;

    /*
     * Note, that it's okay to mix named parameters with the same name but
     * different types, for example:
     * select :"table" from :{table} t where t.tname = :'table'.
     */
  } else
    throw Generic_exception{"maximum parameters count (" +
      std::to_string(max_parameter_count()) + ") exceeded"};

  assert(is_invariant_ok());
}

// ---------------------------------------------------------------------------
// Statement updaters
// ---------------------------------------------------------------------------

// Exception safety guarantee: basic.
DMITIGR_PGFE_INLINE void Statement::update_cache(const Statement& rhs)
{
  // Prepare positional parameters for merge.
  const auto old_pos_params_size = positional_parameters_.size();
  const auto rhs_pos_params_size = rhs.positional_parameters_.size();
  const auto new_pos_params_size = std::max(old_pos_params_size, rhs_pos_params_size);
  positional_parameters_.resize(new_pos_params_size); // can throw

  // Recreate the cache for named parameters. (Can throw.)
  named_parameters_ = named_parameters();

  // Check the new parameter count.
  const auto new_parameter_count = new_pos_params_size + named_parameters_.size();
  if (new_parameter_count > max_parameter_count())
    throw Generic_exception{"parameter count (" +
      std::to_string(new_parameter_count) + ") "
      "exceeds the maximum (" + std::to_string(max_parameter_count()) + ")"};

  // Merge positional parameters (cannot throw).
  for (std::size_t i{}; i < rhs_pos_params_size; ++i)
    positional_parameters_[i] += rhs.positional_parameters_[i];

  assert(is_invariant_ok());
}

// ---------------------------------------------------------------------------
// Statement helpers
// ---------------------------------------------------------------------------

DMITIGR_PGFE_INLINE auto
Statement::named_parameter_type(const std::size_t index) const noexcept
  -> Fragment::Type
{
  DMITIGR_ASSERT(positional_parameter_count() <= index && index < parameter_count());
  const auto relative_index = index - positional_parameter_count();
  return named_parameters_[relative_index].type;
}

DMITIGR_PGFE_INLINE std::size_t
Statement::named_parameter_index(const std::string_view name) const noexcept
{
  const auto relative_index = [this, name]() noexcept
  {
    const auto b = cbegin(named_parameters_);
    const auto e = cend(named_parameters_);
    const auto i = find_if(b, e, [name](const auto& p) noexcept
    {
      return p.name == name;
    });
    return static_cast<std::size_t>(i - b);
  }();
  return positional_parameter_count() + relative_index;
}

DMITIGR_PGFE_INLINE auto
Statement::named_parameters() const -> std::vector<Named_parameter>
{
  std::vector<Named_parameter> result;
  result.reserve(fragments_.size() -
    (!fragments_.empty() && !fragments_.front().is_named_parameter()));
  const auto fragments_end = cend(fragments_);
  for (auto i = cbegin(fragments_); i != fragments_end; ++i) {
    if (i->is_named_parameter()) {
      const auto e = end(result);
      const auto p = find_if(begin(result), e, [i](const auto& param) noexcept
      {
        return param.name == i->str;
      });
      if (p == e)
        result.emplace_back(i->type, i->str);
      else
        ++p->count;
    }
  }
  return result;
}

// -----------------------------------------------------------------------------
// Statement's basic SQL parser
// -----------------------------------------------------------------------------

/*
 * SQL SYNTAX BASICS (from PostgreSQL documentation):
 * https://www.postgresql.org/docs/current/static/sql-syntax-lexical.html
 *
 * COMMANDS
 *
 * A command is composed of a sequence of tokens, terminated by a (";").
 * A token can be a key word, an identifier, a quoted identifier,
 * a literal (or constant), or a special character symbol. Tokens are normally
 * separated by whitespace (space, tab, newline), but need not be if there is no
 * ambiguity.
 *
 * IDENTIFIERS (UNQUOTED)
 *
 * SQL identifiers and key words must begin with a letter (a-z, but also
 * letters with diacritical marks and non-Latin letters) or an ("_").
 * Subsequent characters in an identifier or key word can be letters,
 * underscores, digits (0-9), or dollar signs ($).
 *
 * QUOTED IDENTIFIERS
 *
 * The delimited identifier or quoted identifier is formed by enclosing an
 * arbitrary sequence of characters in double-quotes ("). Quoted identifiers can
 * contain any character, except the character with code zero. (To include a
 * double quote, two double quotes should be written.)
 *
 * CONSTANTS
 *
 *   STRING CONSTANTS (QUOTED LITERALS)
 *
 * A string constant in SQL is an arbitrary sequence of characters bounded
 * by single quotes ('), for example 'This is a string'. To include a
 * single-quote character within a string constant, write two adjacent
 * single quotes, e.g., 'Dianne''s horse'.
 *
 *   DOLLAR QUOTED STRING CONSTANTS
 *
 * A dollar-quoted string constant consists of a dollar sign ($), an
 * optional "tag" of zero or more characters, another dollar sign, an
 * arbitrary sequence of characters that makes up the string content, a
 * dollar sign, the same tag that began this dollar quote, and a dollar
 * sign.
 * The tag, if any, of a dollar-quoted string follows the same rules
 * as an unquoted identifier, except that it cannot contain a dollar sign.
 * A dollar-quoted string that follows a keyword or identifier must be
 * separated from it by whitespace; otherwise the dollar quoting delimiter
 * would be taken as part of the preceding identifier.
 *
 * SPECIAL CHARACTERS
 *
 * - A dollar sign ("$") followed by digits is used to represent a positional
 * parameter in the body of a function definition or a prepared statement.
 * In other contexts the dollar sign can be part of an identifier or a
 * dollar-quoted string constant.
 *
 * - The colon (":") is used to select "slices" from arrays. In certain SQL
 * dialects (such as Embedded SQL), the colon is used to prefix variable
 * names.
 * [In Pgfe ":" is used to prefix named parameters and placeholders.]
 */

/// @returns Preparsed statement and the index of a character that follows it.
DMITIGR_PGFE_INLINE std::pair<Statement, std::string_view::size_type>
Statement::parse_sql_input(const std::string_view text)
{
  if (!text.data())
    throw Generic_exception{"cannot parse invalid SQL input"};
  else if (text.empty())
    return {Statement{}, 1u};

  enum {
    invalid,

    top,

    colon,
    named_parameter,

    dollar,
    positional_parameter,
    dollar_quote_leading_tag,
    dollar_quote,
    dollar_quote_dollar,

    quote,
    quote_quote,

    dash,
    one_line_comment,

    slash,
    multi_line_comment,
    multi_line_comment_asterisk
  } state = top;

  Statement result;
  int multi_line_comment_depth{};
  int fragment_depth{};
  char current_char{};
  char previous_char{};
  char quote_char{};
  std::string fragment;
  std::string dollar_quote_leading_tag_name;
  std::string dollar_quote_trailing_tag_name;
  const auto b = cbegin(text);
  auto i = b;
  for (const auto e = cend(text); i != e; previous_char = current_char, ++i) {
    current_char = *i;
  start:
    switch (state) {
    case top:
      switch (current_char) {
      case '\'':
        [[fallthrough]];
      case '"':
        state = quote;
        quote_char = current_char;
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        fragment += current_char;
        continue;

      case '(':
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        ++fragment_depth;
        fragment += current_char;
        continue;

      case ')':
        fragment += current_char;
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        --fragment_depth;
        if (fragment_depth < 0)
          state = invalid;
        continue;

      case '$':
        if (!is_ident_char(previous_char))
          state = dollar;
        else if (previous_char == '$')
          state = invalid;
        else
          fragment += current_char;
        continue;

      case ':':
        state = colon;
        continue;

      case '-':
        state = dash;
        continue;

      case '/':
        state = slash;
        continue;

      case ';':
        goto finish;

      default:
        fragment += current_char;
        continue;
      } // switch (current_char)

    case dollar:
      DMITIGR_ASSERT(previous_char == '$');
      if (std::isdigit(static_cast<unsigned char>(current_char))) {
        state = positional_parameter;
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        // The 1st digit of positional parameter (current_char) will be stored below.
      } else if (is_ident_char(current_char)) {
        if (current_char == '$') {
          state = dollar_quote;
        } else {
          state = dollar_quote_leading_tag;
          dollar_quote_leading_tag_name += current_char;
        }
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        fragment += previous_char; // $
      } else {
        state = top;
        fragment += previous_char; // $
      }
      fragment += current_char;
      continue;

    case positional_parameter:
      DMITIGR_ASSERT(isdigit(static_cast<unsigned char>(previous_char)));
      if (!isdigit(static_cast<unsigned char>(current_char))) {
        state = !fragment.empty() ? top : invalid;
        result.push_positional_parameter(fragment_depth, fragment);
        fragment.clear();
      }

      if (current_char != ';')
        goto start;
      else
        goto finish;

    case dollar_quote_leading_tag:
      DMITIGR_ASSERT(previous_char != '$' && is_ident_char(previous_char));
      if (current_char == '$') {
        fragment += current_char;
        state = dollar_quote;
      } else if (is_ident_char(current_char)) {
        dollar_quote_leading_tag_name += current_char;
        fragment += current_char;
      } else
        goto finish;
      continue;

    case dollar_quote:
      if (current_char == '$')
        state = dollar_quote_dollar;
      fragment += current_char;
      continue;

    case dollar_quote_dollar:
      if (current_char == '$') {
        if (dollar_quote_leading_tag_name == dollar_quote_trailing_tag_name) {
          state = top;
          dollar_quote_leading_tag_name.clear();
          dollar_quote_trailing_tag_name.clear();
          fragment += current_char;
          result.push_quoted_text(fragment_depth, fragment);
          fragment.clear();
          continue;
        } else {
          state = dollar_quote;
          dollar_quote_trailing_tag_name.clear();
          goto start;
        }
      } else if (is_ident_char(current_char)) {
        dollar_quote_trailing_tag_name += current_char;
      } else {
        state = dollar_quote;
        dollar_quote_trailing_tag_name.clear();
      }
      fragment += current_char;
      continue;

    case colon:
      DMITIGR_ASSERT(previous_char == ':');
      if (current_char == '{' || is_quote_char(current_char)) {
        state = named_parameter;
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        if (is_quote_char(current_char))
          quote_char = current_char;
        continue;
      } else {
        state = top;
        fragment += previous_char;
        goto start;
      }

    case named_parameter:
      DMITIGR_ASSERT(!previous_char || previous_char == '{' ||
        (is_quote_char(previous_char) && quote_char) ||
        is_named_param_char(previous_char));

      if (previous_char == '{' &&
        !std::isalpha(static_cast<unsigned char>(current_char)))
        goto finish;
      else if (!is_named_param_char(current_char)) {
        if (current_char == '}' || current_char == quote_char) {
          state = !fragment.empty() ? top : invalid;
          result.push_named_parameter(fragment_depth, fragment, quote_char);
          fragment.clear();
          quote_char = 0;
          continue;
        } else
          goto finish;
      } else {
        fragment += current_char;
        continue;
      }

    case quote:
      if (current_char == quote_char)
        state = quote_quote;
      else
        fragment += current_char;
      continue;

    case quote_quote:
      DMITIGR_ASSERT(previous_char == quote_char);
      if (current_char != quote_char) {
        state = top;
        quote_char = 0;
        fragment += previous_char; // store previous quote
        result.push_quoted_text(fragment_depth, fragment);
        fragment.clear();
        goto start;
      } else {
        state = quote;
        fragment += current_char; // skip previous, store current quote
        continue;
      }

    case dash:
      DMITIGR_ASSERT(previous_char == '-');
      if (current_char == '-') {
        state = one_line_comment;
        result.push_text(fragment_depth, fragment);
        fragment.clear();
        // The comment marker ("--") will not be included in the next fragment.
      } else {
        state = top;
        fragment += previous_char;

        if (current_char != ';')
          goto start;
        else
          goto finish;
      }
      continue;

    case one_line_comment:
      if (current_char == '\n') {
        state = top;
        if (!fragment.empty() && fragment.back() == '\r')
          fragment.pop_back();
        result.push_one_line_comment(fragment_depth, fragment);
        fragment.clear();
      } else
        fragment += current_char;
      continue;

    case slash:
      DMITIGR_ASSERT(previous_char == '/');
      if (current_char == '*') {
        state = multi_line_comment;
        if (multi_line_comment_depth > 0) {
          fragment += previous_char; // '/'
          fragment += current_char;
        } else {
          result.push_text(fragment_depth, fragment);
          fragment.clear();
          // The comment marker ("/*") will not be included in the next fragment.
        }
        ++multi_line_comment_depth;
      } else {
        state = (multi_line_comment_depth == 0) ? top : multi_line_comment;
        fragment += previous_char; // '/'
        fragment += current_char;
      }
      continue;

    case multi_line_comment:
      if (current_char == '/')
        state = slash;
      else if (current_char == '*')
        state = multi_line_comment_asterisk;
      else
        fragment += current_char;
      continue;

    case multi_line_comment_asterisk:
      DMITIGR_ASSERT(previous_char == '*');
      if (current_char == '/') {
        --multi_line_comment_depth;
        DMITIGR_ASSERT(multi_line_comment_depth >= 0);
        if (multi_line_comment_depth == 0) {
          state = top;
          // Without trailing "*/".
          result.push_multi_line_comment(fragment_depth, fragment);
          fragment.clear();
        } else {
          state = multi_line_comment;
          fragment += previous_char; // '*'
          fragment += current_char;  // '/'
        }
      } else {
        state = multi_line_comment;
        fragment += previous_char;
        fragment += current_char;
      }
      continue;

    case invalid:
      goto finish;
    } // switch (state)
  } // for

 finish:
  if (fragment_depth != 0)
    state = invalid;
  switch (state) {
  case top:
    if (!fragment.empty())
      result.push_text(fragment_depth, fragment);
    break;
  case quote_quote:
    fragment += previous_char;
    result.push_quoted_text(fragment_depth, fragment);
    break;
  case one_line_comment:
    result.push_one_line_comment(fragment_depth, fragment);
    break;
  case positional_parameter:
    result.push_positional_parameter(fragment_depth, fragment);
    break;
  default: {
    std::string message{"invalid SQL input:\n"};
    message.append(text);
    if (!result.fragments_.empty())
      message.append("\n  after: ").append(result.fragments_.back().str);
    throw Generic_exception{message};
  }
  }

  ++i;
  return {std::move(result), i - b};
}

} // namespace dmitigr::pgfe
