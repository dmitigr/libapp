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

#ifndef DMITIGR_PGFE_STATEMENT_HPP
#define DMITIGR_PGFE_STATEMENT_HPP

#include "../base/assert.hpp"
#include "../base/assoc_vector.hpp"
#include "../base/string_vector.hpp"
#include "basics.hpp"
#include "dll.hpp"
#include "exceptions.hpp"
#include "parameterizable.hpp"
#include "types_fwd.hpp"

#include <any>
#include <compare>
#include <cstddef>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dmitigr::pgfe {

/**
 * @ingroup utilities
 *
 * @brief A preparsed statement.
 *
 * @details A dollar sign ("$") followed by digits is used to denote a parameter
 * with explicitly specified position. A colon (":") followed by an opening curly
 * bracket, or single or double quote, followed by alphabetic character, followed
 * by mix of alphabetic characters underscores and dashes is used to denote a
 * named parameter with automatically assignable position, for example:
 *   - `:{this-is-a-valid_name}` - denotes a valid name;
 *   - `:{-this-is-an-invalid_name}` - denotes an invalid name.
 * The valid parameter positions range is `[1, max_parameter_count()]`.
 *
 * Quoting the name of named parameter with either single or double quotes will
 * lead to automatically quoting the bound content of such a parameter as a
 * literal or an identifier accordingly at the time of generating the resulting
 * query string with to_query_string().
 *
 * Examples of valid statements:
 *
 *   - the statement without parameter:
 *     @code{sql} SELECT 1 @endcode
 *
 *   - the statement with the both positional and named parameters:
 *     @code{sql} SELECT 2, $1::int, :{name}::text @endcode
 *
 *   - the statement with named parameter:
 *     @code{sql} WHERE :{name} = 'Dmitry Igrishin' @endcode
 *
 *   - the statement with quoted named parameters:
 *     @code{sql} SELECT :'text' AS :"name" @endcode
 */
class Statement final : public Parameterizable {
public:
  /// An alias of extra data.
  using Extra_data = Assoc_vector<std::string, std::any>;

  /// An alias of destructured string.
  using Destructured_string = String_vector<std::string_view>;

  /// @name Constructors
  /// @{

  /// Default-constructible. (Constructs an empty instance.)
  Statement() = default;

  /**
   * @brief The constructor.
   *
   * @param text Any part of SQL statement, which may contain multiple
   * commands and comments. Comments can contain an associated extra data.
   *
   * @remarks While the SQL input may contain multiple commands, the parser
   * stops on either first top-level semicolon or zero character.
   *
   * @see extra().
   */
  DMITIGR_PGFE_API Statement(std::string_view text);

  /// @overload
  DMITIGR_PGFE_API Statement(const std::string& text);

  /// @overload
  DMITIGR_PGFE_API Statement(const char* text);

  /// @}

  /// @see Parameterizable::positional_parameter_count().
  DMITIGR_PGFE_API std::size_t positional_parameter_count() const noexcept override;

  /// @see Parameterizable::named_parameter_count().
  DMITIGR_PGFE_API std::size_t named_parameter_count() const noexcept override;

  /// @see Parameterizable::parameter_count().
  DMITIGR_PGFE_API std::size_t parameter_count() const noexcept override;

  /// @see Parameterizable::has_positional_parameter().
  DMITIGR_PGFE_API bool has_positional_parameter() const noexcept override;

  /// @see Parameterizable::has_named_parameter().
  DMITIGR_PGFE_API bool has_named_parameter() const noexcept override;

  /// @see Parameterizable::has_parameter().
  DMITIGR_PGFE_API bool has_parameter() const noexcept override;

  /// @see Parameterizable::has_parameter(std::string_view).
  DMITIGR_PGFE_API bool has_parameter(std::string_view) const noexcept override;

  /// @see Parameterizable::parameter_name().
  DMITIGR_PGFE_API std::string_view
  parameter_name(const std::size_t index) const override;

  /// @see Parameterizable::parameter_index().
  DMITIGR_PGFE_API std::size_t
  parameter_index(const std::string_view name) const noexcept override;

  /// @returns `true` if this statement is empty.
  DMITIGR_PGFE_API bool is_empty() const noexcept;

  /**
   * @returns `true` if this statement is consists only of comments and blank
   * line(-s).
   */
  DMITIGR_PGFE_API bool is_query_empty() const noexcept;

  /**
   * @returns `false` if the parameter at specified `index` is missing. For
   * example, the statement
   * @code{sql} SELECT :{p}, $3 @endcode
   * has two missing parameters at indexes `0` and `1`.
   *
   * @par Requires
   * `index < positional_parameter_count()`.
   *
   * @remarks Missing parameter can only be eliminated by using methods append()
   * or replace(). Thus, by replacing the parameter `p` with `$2, $1` in the
   * example above, missing parameters will be eliminated because the statement
   * will become the following:
   * @code{sql} SELECT $2, $1, $3 @endcode
   *
   * @see append(), replace().
   */
  DMITIGR_PGFE_API bool
  is_parameter_missing(const std::size_t index) const;

  /**
   * @returns `true` if the parameter at specified `index` represents the
   * literal and can be bound with the value for further quoting (escaping).
   *
   * @par Requires
   * `index` in range `[positional_parameter_count(), parameter_count())`.
   *
   * @see bind().
   */
  DMITIGR_PGFE_API bool
  is_parameter_literal(const std::size_t index) const;

  /// @overload
  DMITIGR_PGFE_API bool
  is_parameter_literal(const std::string_view name) const;

  /**
   * @returns `true` if the parameter at specified `index` represents the
   * identifier and can be bound with the value for further quoting (escaping).
   *
   * @par Requires
   * `index` in range `[positional_parameter_count(), parameter_count())`.
   *
   * @see bind().
   */
  DMITIGR_PGFE_API bool
  is_parameter_identifier(const std::size_t index) const;

  /// @overload
  DMITIGR_PGFE_API bool
  is_parameter_identifier(const std::string_view name) const;

  /**
   * @returns `true` if this statement has a positional parameter with the
   * index `i` such that `!is_parameter_missing(i)`.
   *
   * @see is_parameter_missing().
   */
  DMITIGR_PGFE_API bool has_missing_parameter() const noexcept;

  /**
   * @returns `true` if there is a named parameter in this statement that
   * appears more than once.
   */
  DMITIGR_PGFE_API bool has_duplicate_named_parameter() const noexcept;

  /**
   * @brief Appends the specified statement.
   *
   * @par Effects
   * This instance contains the given `appendix`. If `is_query_empty()` before
   * calling this method, then extra data of `appendix` is appended to the extra
   * data of this instance.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  DMITIGR_PGFE_API void append(const Statement& appendix);

  /**
   * @brief Binds the parameter named by the `name` with the specified `value`.
   *
   * @returns `*this`.
   *
   * @par Requires
   * `has_parameter(name)`.
   *
   * @par Effects
   * The parameter `name` is associated with the given `value` which will be used
   * as the parameter substitution upon of calling to_query_string().
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see unbind(), has_parameter(), replace(), bound().
   */
  DMITIGR_PGFE_API Statement& bind(const std::string& name, std::string value);

  /**
   * @brief Unbinds the parameter named by the `name` from the associated value.
   *
   * @returns `*this`.
   *
   * @par Requires
   * `has_parameter(name)`.
   *
   * @par Effects
   * The parameter `name` is not associated with value and will not be substituted
   * upon of calling to_query_string().
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see bind(), has_parameter(), replace(), bound().
   */
  DMITIGR_PGFE_API Statement& unbind(const std::string& name);

  /**
   * @returns The value bound to parameter, or `nullptr` if no value bound.
   *
   * @par Requires
   * `has_parameter(name)`.
   *
   * @see bind().
   */
  DMITIGR_PGFE_API const std::string* bound(const std::string& name) const;

  /**
   * @returns The number of bound parameters.
   *
   * @see has_bound_parameter(), bound().
   */
  DMITIGR_PGFE_API std::size_t bound_parameter_count() const noexcept;

  /**
   * @returns `true` if `bound_parameter_count() > 0`.
   *
   * @see bound_parameter_count(), bound().
   */
  DMITIGR_PGFE_API bool has_bound_parameter() const noexcept;

  /**
   * @returns `true` if `bound_parameter_count() < named_parameter_count()`.
   *
   * @see bound_parameter_count(), named_parameter_count().
   */
  DMITIGR_PGFE_API bool has_unbound_parameter() const noexcept;

  /**
   * @brief Replaces the parameter named by the `name` with the specified
   * `replacement`.
   *
   * @par Requires
   * `has_parameter(name) && std::addressof(replacement) != this`.
   *
   * @par Effects
   * This instance contains the given `replacement` instead of the parameter
   * named by the `name`. The extra data will *not* be affected.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see has_parameter(), bind().
   */
  DMITIGR_PGFE_API void replace(std::string_view name, const Statement& replacement);

  /**
   * @returns The result of conversion of this instance to the instance of
   * type `std::string`.
   */
  DMITIGR_PGFE_API std::string to_string() const;

  /**
   * @returns The query string that's actually passed to a PostgreSQL server.
   *
   * @par Requires
   * `!has_missing_parameter() && conn.is_connected()`.
   */
  DMITIGR_PGFE_API std::string to_query_string(const Connection& conn) const;

  /// @returns The extra data associated with this instance.
  ///
  /// @details An any data can be associated with an object of type Statement.
  /// The initial associations can be specified in the *related comments*. The
  /// related comment - is comment that have no more than one newline
  /// character in between themself and the following content. The content
  /// following the related comments must not be empty or consists only of spaces.
  ///
  /// Consider the example of the SQL input:
  /// @code{sql}
  /// -- This is the unrelated comment (because 2 new line feeds follows after it).
  /// -- $id$unrelated$id$
  ///
  /// -- This is the related one line comment 1
  /// -- $id$select-all$id$
  /// /* $where$
  ///  * num > 0
  ///  * AND num < :{num}
  ///  * $where$
  ///  */
  ///  -- This is the related one line comment 2
  /// SELECT * FROM table WHERE :{where};
  /// @endcode
  /// The SQL code above contains just one actual query:
  /// @code{sql}SELECT * FROM table WHERE :{where}@endcode
  /// This query has seven related comments and two unrelated comments (at the
  /// beginning) because there are two newline characters following them. Next,
  /// there are two data associations specified as a dollar-quoted string
  /// constants tagged as `id` and `where`. The valid characters of the tags
  /// are: alphanumerics, the underscore character and the dash.
  /// Please, note, that the content in between the named tags might consist to
  /// multiple lines. There are rules of the content formatting in such cases:
  ///   -# The leading and trailing newline characters are always ignored and
  ///   other newline characters are always preserved;
  ///   -# If the content begins with non newline character, then the content is
  ///   associated exactly as provided, i.e. all indentations are preserved;
  ///   -# If the content begins with a newline character then the following
  ///   lines will be left-aligned relative the *most left non space character*.
  ///   In case of the sequence of one-line comments, the most left non space
  ///   character are always follows the one-line comment marker ("--"). In case
  ///   of the multi-line comment, the most left non space character can be a
  ///   character that follows the asterisk with a space ("* "), or just the
  ///   most left character.
  ///
  /// Examples:
  ///
  /// Example 1. The misaligned content of the association specified in the
  /// multi-line comment
  ///
  /// @code{sql}
  /// /*
  ///  * $text1$
  ///    * one
  ///      * two
  ///    * three
  ///  * $text1$
  ///  */
  /// SELECT 1, 2, 3
  /// @endcode
  ///
  /// The content of the `text1` association is "one\n  * two\nthree".
  ///
  /// Example 2. The aligned content of the association specified in the
  /// multi-line comment
  ///
  /// @code{sql}
  /// /*
  ///  * $text2$
  ///  * one
  ///  * two
  ///  * three
  ///  * $text2$
  ///  */
  /// SELECT 1, 2, 3
  /// @endcode
  ///
  /// The content of the `text2` association is "one\ntwo\nthree".
  ///
  /// Example 3. The content of the association specified in the sequence of
  /// one-line comments
  ///
  /// @code{sql}
  /// -- $text3$
  /// --one
  /// -- two
  /// -- three
  /// -- $text3$
  /// SELECT 1, 2, 3
  /// @endcode
  ///
  /// The content of the `text3` association is "one\n two\n three".
  DMITIGR_PGFE_API const Extra_data& extra() const;

  /// @overload
  DMITIGR_PGFE_API Extra_data& extra();

  /**
   * @brief Tests instances on equivalency.
   *
   * @details Statements are equivalent if all theirs parts, except comments and
   * named parameters, being normalized are equal.
   *
   * @returns `true` if this instance equivalents to `rhs`.
   */
  DMITIGR_PGFE_API bool is_equivalent(const Statement& rhs) const;

  /**
   * @returns `true` if the named parameters of this statement are equals to the
   * named parameters of `rhs`.
   */
  DMITIGR_PGFE_API bool are_named_parameters_equal(const Statement& rhs) const;

  /// @returns `is_equivalent(rhs) && is_named_parameters_equal(rhs)`.
  DMITIGR_PGFE_API bool is_equal(const Statement& rhs) const;

  /**
   * @brief Destructures this statement according to the `pattern`.
   *
   * @param callback A function with signature
   * `bool callback(const std::string& name, const Destructured_string&)`
   * which called for every matching found.
   *
   * @par Requires
   *   -# `!has_unbound_parameter()`;
   *   -# `pattern` must not have adjacent named parameters without non-space
   *   text between them.
   *
   * @returns `true` if this statement matches to the `pattern` and it's parts
   * are destructured by the named parameters of the `pattern`.
   *
   * @see bind().
   */
  template<typename F>
  bool destructure(F&& callback, const Statement& pattern) const
  {
    static const auto make_view = [](const std::string& str,
      const std::string::size_type begin,
      const std::string::size_type end) noexcept
    {
      DMITIGR_ASSERT(begin <= end);
      DMITIGR_ASSERT(begin <= str.size());
      DMITIGR_ASSERT(end - begin <= str.size() - begin);
      return std::string_view{str.data() + begin, end - begin};
    };

    static const auto push_back_if_not_empty = [](auto& cont, auto&& val)
    {
      if (val.data() && !val.empty())
        cont.push_back(std::forward<decltype(val)>(val));
    };

    thread_local Assoc_vector<const std::string*, Destructured_string> result(16);
    result.clear();

    normalize();
    pattern.normalize();
    auto nf = norm_fragments_.cbegin();
    auto pnf = pattern.norm_fragments_.cbegin();
    const auto nf_end = norm_fragments_.cend();
    const auto pnf_end = pattern.norm_fragments_.cend();
    std::string::size_type nf_norm_offset{};
    const auto shift_nf_norm_offset = [&nf, &pnf, &nf_norm_offset]
      (const std::string::size_type norm_pos)
    {
      nf_norm_offset = nf->norm_str()
        .find_first_not_of(' ', norm_pos + pnf->norm_str().size());
      if (nf_norm_offset == std::string::npos) {
        nf_norm_offset = 0;
        ++nf;
      }
    };

    while (nf != nf_end && pnf != pnf_end) {
      if (pnf->is_named_parameter()) {
        const auto& name = pnf->str;

        // Skip empty text fragments which follows the parameter name.
        ++pnf;
        while (pnf != pnf_end && pnf->is_text() && pnf->norm_str().empty())
          ++pnf;
        if (pnf != pnf_end && pnf->is_named_parameter())
          throw Generic_exception{"cannot destructure statement: pattern has "
            "adjacent named parameters without non-space text between them: "+
            name+" and "+pnf->str};

        Destructured_string matching;
        matching.reserve(16);
        for (; nf != nf_end; ++nf) {
          if (nf->is_text()) {
            if (pnf != pnf_end && pnf->is_text()) {
              if (nf->depth == pnf->depth) {
                DMITIGR_ASSERT(!nf->norm_str().empty());
                DMITIGR_ASSERT(!pnf->norm_str().empty());
                const auto norm_pos = nf->norm_str().find(pnf->norm_str(), nf_norm_offset);
                if (norm_pos != std::string::npos) {
                  const auto end = norm_pos - ((norm_pos > nf_norm_offset) &&
                    nf->norm_str()[norm_pos - 1] == ' ');
                  push_back_if_not_empty(matching,
                    make_view(nf->norm_str(), nf_norm_offset, end));
                  shift_nf_norm_offset(norm_pos);
                  ++pnf;
                  break;
                }
              } else if (nf->depth < pnf->depth)
                return false;
            }
            push_back_if_not_empty(matching,
              make_view(nf->norm_str(), nf_norm_offset, nf->norm_str().size()));
          } else if (nf->is_named_parameter()) {
            if (const auto* const b = bound(nf->str))
              push_back_if_not_empty(matching,
                std::string_view{b->data(), b->size()});
            else
              throw Generic_exception{"cannot destructure statement: it has "
                "unbound parameter "+nf->str};
          } else if (pnf != pnf_end && nf->norm_equal(*pnf)) {
            ++nf;
            ++pnf;
            break;
          } else
            push_back_if_not_empty(matching, nf->str);
        } // for

        if (!matching.is_empty())
          result.append(std::addressof(name), std::move(matching));
        else
          return false;
      } else if (pnf->is_text()) {
        if (nf->depth != pnf->depth)
          return false;
        const auto& nf_norm_str = nf->norm_str();
        const auto& pnf_norm_str = pnf->norm_str();
        if (pnf_norm_str.empty())
          return nf_norm_str.empty();
        const auto norm_pos = nf_norm_str.find(pnf_norm_str);
        if (norm_pos == std::string::npos)
          return false;
        shift_nf_norm_offset(norm_pos);
        ++pnf;
      } else if (pnf->norm_equal(*nf)) {
        ++nf;
        ++pnf;
      } else
        return false;
    } // while
    if (!(nf == nf_end && pnf == pnf_end))
      return false;

    for (const auto& [name, mtch] : result.vector()) {
      DMITIGR_ASSERT(name);
      DMITIGR_ASSERT(!mtch.is_empty());
      if (!callback(*name, mtch))
        break;
    }
    return true;
   }

  /**
   * @brief Normalizes this statement.
   *
   * @details This is called implicitly upon of calling such functions as
   * is_equivalent() or destructure().
   *
   * @par Exception safety guarantee
   * Strong.
   */
  DMITIGR_PGFE_API void normalize() const;

  /**
   * @returns `true` if this statement is normalized.
   *
   * @see normalize().
   */
  DMITIGR_PGFE_API bool is_normalized() const noexcept;

private:
  friend Statement_vector;

  /// A fragment.
  struct Fragment final {
    enum class Type {
      text,
      quoted_text,
      one_line_comment,
      multi_line_comment,
      named_parameter,
      named_parameter_literal,
      named_parameter_identifier,
      positional_parameter
    };

    Fragment(const Type tp, const int depth, const std::string& s);

    bool is_text() const noexcept;
    bool is_quoted_text() const noexcept;
    bool is_named_parameter() const noexcept;
    bool is_named_parameter(const std::string_view name) const noexcept;
    bool is_quoted_named_parameter() const noexcept;
    bool is_quoted_named_parameter(const std::string_view name) const noexcept;
    bool is_positional_parameter() const noexcept;
    bool is_parameter() const noexcept;
    bool is_comment() const noexcept;

    const std::string& norm_str() const;
    bool norm_equal(const Fragment& rhs) const;

    Type type;
    int depth{};
    std::string str;
    mutable std::string norm;
  };

  struct Named_parameter final {
    Fragment::Type type;
    std::string name;
    std::size_t count{1};
    auto operator<=>(const Named_parameter&) const noexcept = default;
  };

  using Fragment_list = std::list<Fragment>;

  Fragment_list fragments_;
  mutable Fragment_list norm_fragments_; // cache
  std::unordered_map<std::string, std::string> bindings_;
  std::vector<bool> positional_parameters_; // cache
  std::vector<Named_parameter> named_parameters_; // cache
  mutable bool is_extra_data_should_be_extracted_from_comments_{true};
  mutable std::optional<Extra_data> extra_; // cache

  static std::pair<Statement, std::string_view::size_type>
  parse_sql_input(std::string_view);

  bool is_invariant_ok() const noexcept override;

  // ---------------------------------------------------------------------------
  // Initializers
  // ---------------------------------------------------------------------------

  void push_back_fragment(Fragment::Type type, int depth, const std::string& str);
  void push_text(int depth, const std::string& str);
  void push_quoted_text(int depth, const std::string& str);
  void push_one_line_comment(int depth, const std::string& str);
  void push_multi_line_comment(int depth, const std::string& str);
  void push_positional_parameter(int depth, const std::string& str);
  void push_named_parameter(int depth, const std::string& str, char quote_char);

  // ---------------------------------------------------------------------------
  // Updaters
  // ---------------------------------------------------------------------------

  // Exception safety guarantee: strong.
  void update_cache(const Statement& rhs);

  // ---------------------------------------------------------------------------
  // Named parameters helpers
  // ---------------------------------------------------------------------------

  Fragment::Type named_parameter_type(const std::size_t index) const noexcept;
  std::size_t named_parameter_index(const std::string_view name) const noexcept;
  std::vector<Named_parameter> named_parameters() const;

  // ---------------------------------------------------------------------------
  // Predicates
  // ---------------------------------------------------------------------------

  static bool is_ident_char(const unsigned char c) noexcept;
  static bool is_named_param_char(const unsigned char c) noexcept;
  static bool is_quote_char(const unsigned char c) noexcept;

  // ---------------------------------------------------------------------------
  // Extra data
  // ---------------------------------------------------------------------------

  /// Represents an API for extraction the extra data from the comments.
  struct Extra;
};

} // namespace dmitigr::pgfe

#ifndef DMITIGR_PGFE_NOT_HEADER_ONLY
#include "statement.cpp"
#endif

#endif  // DMITIGR_PGFE_STATEMENT_HPP
