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

#ifndef DMITIGR_MULF_FORM_DATA_HPP
#define DMITIGR_MULF_FORM_DATA_HPP

#include "../base/assert.hpp"
#include "../base/str.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace dmitigr::mulf {

/// An entry of multipart/form-data.
class Form_data_entry final {
public:
  /// The constructor.
  explicit Form_data_entry(std::string name)
    : name_{std::move(name)}
  {
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The name of the entry.
   *
   * @see set_name().
   */
  const std::string& name() const noexcept
  {
    return name_;
  }

  /**
   * @brief Sets the name of the entry.
   *
   * @par Requires
   * `!name.empty()`.
   *
   * @see name().
   */
  void set_name(std::string name)
  {
    if (name.empty())
      throw Exception{"cannot set empty name to multipart/form-data entry"};

    name_ = std::move(name);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The filename of the entry.
   *
   * @see set_filename().
   */
  const std::optional<std::string>& filename() const
  {
    return filename_;
  }

  /**
   * @brief Sets the filename of the entry.
   *
   * @par Requires
   * `(!filename || !filename->empty())`.
   *
   * @see filename().
   */
  void set_filename(std::optional<std::string> filename)
  {
    if (filename && filename->empty())
      throw Exception{"cannot set empty filename to multipart/form-data entry"};

    filename_ = std::move(filename);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The content type of the entry.
   *
   * @see set_content_type().
   */
  const std::optional<std::string>& content_type() const noexcept
  {
    return content_type_;
  }

  /**
   * @brief Sets the content type of the entry.
   *
   * @par Requires
   * `(!content_type || !content_type->empty())`.
   *
   * @see content_type().
   */
  void set_content_type(std::optional<std::string> content_type)
  {
    if (content_type && content_type->empty())
      throw Exception{"cannot set empty content type to multipart/form-data entry"};

    content_type_ = std::move(content_type);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The charset of the entry.
   *
   * @see set_charset().
   */
  const std::optional<std::string>& charset() const noexcept
  {
    return charset_;
  }

  /**
   * @brief Sets the charset of the entry.
   *
   * @par Requires
   * `(!charset || !charset->empty())`.
   *
   * @see charset().
   */
  void set_charset(std::optional<std::string> charset)
  {
    if (charset && charset->empty())
      throw Exception{"cannot set empty charset to multipart/form-data entry"};

    charset_ = std::move(charset);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @returns The content of the entry.
   *
   * @see set_content().
   */
  std::optional<std::string_view> content() const noexcept
  {
    static const auto visitor = [](auto& result)
    {
      return std::make_optional(static_cast<std::string_view>(result));
    };
    return content_ ? std::visit(visitor, *content_) : std::nullopt;
  }

  /**
   * @brief Sets the content of the entry.
   *
   * @par Requires
   * `(!view || !view->empty())`.
   *
   * @see content().
   */
  void set_content(std::optional<std::string_view> content)
  {
    if (content && content->empty())
      throw Exception{"cannot set empty content to multipart/form-data entry"};

    content_ = std::move(content);
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * @overload
   *
   * @par Requires
   * `(!content || !content->empty())`.
   *
   * @see content().
   */
  void set_content(std::optional<std::string> content)
  {
    if (content && content->empty())
      throw Exception{"cannot set empty content to multipart/form-data entry"};

    content_ = std::move(content);
    DMITIGR_ASSERT(is_invariant_ok());
  }

private:
  friend class Form_data;

  std::string name_;
  std::optional<std::string> filename_;
  std::optional<std::string> content_type_;
  std::optional<std::string> charset_;
  std::optional<std::variant<std::string, std::string_view>> content_;

  Form_data_entry() = default; // constructs invalid object

  bool is_invariant_ok() const
  {
    const bool name_ok = !name_.empty();
    const bool filename_ok = !filename_ || !filename_->empty();
    const bool content_type_ok = !content_type_ || !content_type_->empty();
    const bool charset_ok = !charset_ || !charset_->empty();
    const bool content_ok = !content_ || !content()->empty();
    return name_ok && filename_ok && content_type_ok && charset_ok && content_ok;
  }
};

/**
 * @brief A parsed multipart/form-data.
 *
 * @remarks Since several entries can be named equally, `offset` can be
 * specified as the starting lookup index in the corresponding methods.
 *
 */
class Form_data final {
public:
  /// The alias of Form_data_entry.
  using Entry = Form_data_entry;

  /**
   * @brief Constructs the object by parsing the multipart/form-data.
   *
   * @param data - the unparsed multipart/form-data;
   * @param boundary - the boundary of multipart/form-data.
   *
   * @remarks The `data` will be used as a storage area to avoid
   * copying the content of the entries (which are can be huge).
   */
  Form_data(std::string data, const std::string& boundary)
    : data_{std::move(data)}
  {
    if (!is_boundary_valid(boundary))
      throw Exception{"invalid multipart/form-data boundary"};

    const auto delimiter{"\r\n--" + boundary};

    auto pos = data_.find(delimiter);
    if (pos != std::string::npos) {
      pos += delimiter.size();
      pos = skip_transport_padding(data_, pos);
      pos = skip_mandatory_crlf(data_, pos);
    } else
      throw Exception{"multipart/form-data contains no boundary"};

    while (true) {
      const auto next_delimiter_pos = data_.find(delimiter, pos);
      if (next_delimiter_pos == std::string::npos)
        throw Exception{"multipart/form-data contains unclosed boundary"};

      DMITIGR_ASSERT(pos < data_.size());

      Entry entry;
      entries_.emplace_back(std::move(entry));
      pos = set_headers(entries_.back(), data_, pos);
      DMITIGR_ASSERT(pos <= next_delimiter_pos);
      if (pos < next_delimiter_pos)
        set_content(entries_.back(), data_, pos, next_delimiter_pos);

      pos = next_delimiter_pos + delimiter.size();
      if (pos + 1 < data_.size()) {
        if (data_[pos] == '-') {
          if (data_[pos + 1] == '-')
            // Close-delimiter found. Don't care about transport padding and epilogue.
            break;
          else
            throw Exception{"multipart/form-data contains invalid close-delimiter"};
        } else {
          pos = skip_transport_padding(data_, pos);
          pos = skip_mandatory_crlf(data_, pos);
        }
      } else
        throw Exception{"multipart/form-data contains no close-delimiter"};
    }
  }

  /// @returns The number of entries.
  std::size_t entry_count() const noexcept
  {
    return entries_.size();
  }

  /// @returns The entry index if `has_entry(name, offset)`.
  std::optional<std::size_t> entry_index(const std::string_view name,
    const std::size_t offset = 0) const noexcept
  {
    if (offset < entry_count()) {
      const auto b = cbegin(entries_);
      const auto e = cend(entries_);
      const auto i = std::find_if(b + offset, e, [&](const auto& entry)
      {
        return entry.name() == name;
      });
      return (i != e) ? std::make_optional(i - b) : std::nullopt;
    } else
      return std::nullopt;
  }

  /**
   * @returns The entry index.
   *
   * @par Requires
   * `has_entry(name, offset)`.
   */
  std::size_t entry_index_throw(const std::string_view name,
    const std::size_t offset = 0) const
  {
    const auto result = entry_index(name, offset);
    DMITIGR_ASSERT(result);
    return *result;
  }

  /**
   * @returns The entry.
   *
   * @par Requires
   * `(index < entry_count())`.
   */
  const Entry& entry(const std::size_t index) const
  {
    if (!(index < entry_count()))
      throw Exception{"cannot get multipart/form-data entry by using invalid index"};

    return entries_[index];
  }

  /**
   * @overload
   *
   * @returns `entry(entry_index_throw(name, offset))`.
   */
  const Entry* entry(const std::string_view name,
    const std::size_t offset = 0) const
  {
    const auto index = entry_index_throw(name, offset);
    return &entries_[index];
  }

  /// @returns `true` if this instance has the entry with the specified `name`.
  bool has_entry(const std::string_view name,
    const std::size_t offset = 0) const noexcept
  {
    return static_cast<bool>(entry_index(name, offset));
  }

  /// @returns `(entry_count() > 0)`.
  bool has_entries() const noexcept
  {
    return !entries_.empty();
  }

private:
  std::string data_;
  std::vector<Entry> entries_;

  /**
   * @returns `true` if the `boundary` contains only allowed characters
   * according to https://tools.ietf.org/html/rfc2046#section-5.1.1.
   */
  static bool is_boundary_valid(const std::string& boundary)
  {
    static const auto is_valid_boundary_character = [](const unsigned char c)
    {
      constexpr const unsigned char allowed[] = {'\'', '(', ')', '+', '_', ',',
        '-', '.', '/', ':', '=', '?', ' '};
      return std::isalnum(c) ||
        std::any_of(std::cbegin(allowed), std::cend(allowed), [c](const auto ch)
        {
          return ch == c;
        });
    };

    return !boundary.empty() && boundary.size() <= 70 &&
      std::all_of(cbegin(boundary), cend(boundary), is_valid_boundary_character);
  }

  /**
   * @returns The position of a character immediately following the transport
   * padding, described at https://tools.ietf.org/html/rfc2046#section-5.1.1,
   * or `pos` if the `data` doesn't starts with such a transport padding.
   */
  static std::string::size_type skip_transport_padding(const std::string& data,
    std::string::size_type pos)
  {
    DMITIGR_ASSERT(pos < data.size());

    bool is_crlf_reached{};
    if (const char c = data[pos]; c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      for (++pos; pos < data.size(); ++pos) {
        const char ch = data[pos];
        switch (ch) {
        case ' ':
        case '\t':
          is_crlf_reached = false;
          continue;
        case '\r':
          if (is_crlf_reached)
            return pos - 2;
          continue;
        case '\n':
          if (is_crlf_reached)
            return pos - 2;
          is_crlf_reached = (data[pos - 1] == '\r');
          continue;
        default:
          goto end;
        }
      }
    }
  end:
    return is_crlf_reached ? pos - 2 : pos;
  }

  /**
   * @returns The position of a character immediately following the `CRLF` sequence.
   *
   * @throws Exception if the `data` doesn't starts with `CRLF`.
   */
  static std::string::size_type skip_mandatory_crlf(const std::string& data,
    const std::string::size_type pos)
  {
    DMITIGR_ASSERT(pos < data.size());

    if (pos + 1 < data.size() && data[pos] == '\r' && data[pos + 1] == '\n')
      return pos + 2;
    else
      throw Exception{"expected CRLF not found in multipart/form-data"};
  }

  /**
   * @brief Parses headers in `data` and stores them in `entry`.
   *
   * @details According to https://tools.ietf.org/html/rfc7578#section-4.5 the
   * example of valid data to parse is:
   *
   *   content-disposition: form-data; name="field1"
   *   content-type: text/plain;charset=UTF-8
   *
   * @returns The position of a character immediately following the `CRLFCRLF`,
   * sequence.
   *
   * @remarks According to https://tools.ietf.org/html/rfc7578#section-4.7
   * the `content-transfer-encoding` header field is deprecated and this
   * implementation doesn't parse it.
   */
  static std::string::size_type set_headers(Entry& entry,
    const std::string& data, std::string::size_type pos)
  {
    DMITIGR_ASSERT(pos < data.size());

    enum { name = 1,
           before_parameter_name,
           parameter_name,
           before_parameter_value,
           parameter_value,
           parameter_quoted_value,
           parameter_quoted_value_bslash,
           parameter_quoted_value_quote,
           cr, crlf, crlfcr, crlfcrlf } state = name;

    enum { content_disposition = 1, content_type } type =
      static_cast<decltype(type)>(0);
    enum { name_parameter = 1, filename_parameter, charset_parameter } param =
      static_cast<decltype(param)>(0);

    std::string extracted;
    bool form_data_extracted{};

    static const auto is_hws_character = [](const unsigned char c)
    {
      return c == ' ' || c == '\t';
    };
    static const auto is_valid_name_character = [](const unsigned char c)
    {
      return std::isalnum(c) || c == '-';
    };
    static const auto is_valid_parameter_name_character = [](const unsigned char c)
    {
      return std::isalnum(c) || c == '-' || c == '/';
    };
    static const auto is_valid_parameter_value_character = [](const unsigned char c)
    {
      // According to https://tools.ietf.org/html/rfc7230#section-3.2.6
      constexpr const unsigned char allowed[] = {'!', '#', '$', '%', '&',
        '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~'};
      return std::isalnum(c) ||
        std::any_of(std::cbegin(allowed), std::cend(allowed), [c](const auto ch)
        {
          return ch == c;
        });
    };
    static const auto is_valid_parameter_quoted_value_character =
      [](const unsigned char c)
    {
      return is_valid_parameter_value_character(c) || is_hws_character(c);
    };

    const auto process_parameter_name = [&](std::string&& extracted) {
      switch (type) {
      case content_disposition:
        if (extracted == "name")
          param = name_parameter;
        else if (extracted == "filename")
          param = filename_parameter;
        else if (extracted == "form-data")
          if (!form_data_extracted)
            form_data_extracted = true;
          else
            throw Exception{"multipart/form-data contains invalid "
              "content-disposition"};
        else
          throw Exception{"multipart/form-data contains invalid "
            "content-disposition"};
        break;
      case content_type:
        if (extracted == "charset")
          param = charset_parameter;
        else if (!entry.content_type_)
          entry.content_type_ = std::move(extracted);
        else
          throw Exception{"multipart/form-data contains invalid content-type"};
        break;
      };
    };

    const auto process_parameter_value = [&](std::string&& extracted) {
      switch (type) {
      case content_disposition:
        if (param == name_parameter)
          entry.name_ = std::move(extracted);
        else if (param == filename_parameter)
          entry.filename_ = std::move(extracted);
        else
          throw Exception{"multipart/form-data contains invalid "
            "content-disposition"};
        break;
      case content_type:
        if (param == charset_parameter)
          entry.charset_ = std::move(extracted);
        else
          throw Exception{"multipart/form-data contains invalid "
            "content-type"};
        break;
      };
    };

    const auto data_size = data.size();
    for (; pos < data_size && state != crlfcrlf; ++pos) {
      const char c = data[pos];
      switch (state) {
      case name:
        if (c == ':') {
          str::lowercase(extracted);
          if (extracted == "content-disposition")
            type = content_disposition;
          else if (extracted == "content-type")
            type = content_type;
          else
            throw Exception{"multipart/form-data contains unallowable or empty "
              "header name"};
          extracted.clear();
          state = before_parameter_name;
          continue; // skip :
        } else if (!is_valid_name_character(c))
          throw Exception{"multipart/form-data contains invalid header name"};
        break;

      case before_parameter_name:
        if (!is_hws_character(c)) {
          if (is_valid_parameter_name_character(c))
            state = parameter_name;
          else
            throw Exception{"multipart/form-data contains invalid header value"};
        } else
          continue; // skip HWS character
        break;

      case parameter_name:
        if (c == ';' || c == '=' || c == '\r') {
          str::lowercase(extracted);
          process_parameter_name(std::move(extracted));
          extracted = {};
          state = (c == ';') ? before_parameter_name
            : (c == '=') ? before_parameter_value : cr;
          continue; // skip ; or = or CR
        } else if (!is_valid_parameter_name_character(c))
          throw Exception{"multipart/form-data contains invalid character in "
            "the header value"};
        break;

      case before_parameter_value:
        if (!is_hws_character(c)) {
          if (c == '"') {
            state = parameter_quoted_value;
            continue; // skip "
          } else if (is_valid_parameter_value_character(c))
            state = parameter_value;
          else
            throw Exception{"multipart/form-data contains invalid header value"};
        } else
          continue; // skip HWS
        break;

      case parameter_value:
        if (is_hws_character(c) || c == '\r') {
          process_parameter_value(std::move(extracted));
          extracted = {};
          state = is_hws_character(c) ? before_parameter_name : cr;
          continue; // skip HWS or CR
        } else if (!is_valid_parameter_value_character(c))
          throw Exception{"multipart/form-data contains invalid header value"};
        break;

      case parameter_quoted_value:
        if (c == '"') {
          state = parameter_quoted_value_quote;
          continue; // skip "
        } else if (c == '\\') {
          state = parameter_quoted_value_bslash;
          continue; // skip back-slash
        } else if (!is_valid_parameter_quoted_value_character(c))
          throw Exception{"multipart/form-data contains invalid header value"};
        break;

      case parameter_quoted_value_quote:
        if (is_hws_character(c) ||
          is_valid_parameter_name_character(c) || c == '\r') {
          process_parameter_value(std::move(extracted));
          extracted = {};
          state = (c != '\r') ? before_parameter_name : cr;
          if (is_hws_character(c) || c == '\r')
            continue; // skip HWS or CR
        } else
          throw Exception{"multipart/form-data contains invalid header value"};
        break;

      case parameter_quoted_value_bslash:
        if (c == '"')
          state = parameter_quoted_value;
        else
          throw Exception{"multipart/form-data contains invalid header value"};
        break;

      case cr:
        if (c == '\n') {
          state = crlf;
          continue; // skip LF
        }
        throw Exception{"expected CRLF not found in multipart/form-data"};

      case crlf:
        if (c == '\r') {
          state = crlfcr;
          continue; // skip CR
        } else
          state = name;
        break;

      case crlfcr:
        if (c == '\n') {
          state = crlfcrlf; // done
          continue; // skip LF
        }
        throw Exception{"expected CRLFCRLF not found in multipart/form-data"};

      case crlfcrlf:
        DMITIGR_ASSERT(false);
      }

      extracted += c;
    }

    if (entry.name().empty() || !form_data_extracted || state != crlfcrlf)
      throw Exception{"multipart/form-data contains invalid MIME-part-headers"};

    DMITIGR_ASSERT(entry.is_invariant_ok());

    return pos;
  }

  /**
   * @brief Creates the view to the `data` by the specified positions and sets
   * it as the content of the `entry`.
   */
  static void set_content(Entry& entry, const std::string& data,
    const std::string::size_type beg, const std::string::size_type end)
  {
    DMITIGR_ASSERT(beg < end && end < data.size());
    entry.content_ = std::string_view{data.data() + beg, end - beg};
    DMITIGR_ASSERT(entry.is_invariant_ok());
  }
};

} // namespace dmitigr::mulf

#endif // DMITIGR_MULF_FORM_DATA_HPP
