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

#ifndef DMITIGR_GPIO_HPP
#define DMITIGR_GPIO_HPP

#include "exceptions.hpp"
#include "version.hpp"

#include <gpiod.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace dmitigr::gpio {

class Edge_event final {
public:
  ~Edge_event()
  {
    if (is_owner_ && handle_) {
      gpiod_edge_event_free(handle_);
      handle_ = nullptr;
    }
  }

  Edge_event() = default;

  Edge_event(const Edge_event& rhs)
    : is_owner_{true}
    , handle_{gpiod_edge_event_copy(rhs.handle_)}
  {
    if (!handle_)
      throw Exception{"cannot copy the edge event object"};
  }

  Edge_event& operator=(const Edge_event& rhs)
  {
    Edge_event tmp{rhs};
    swap(tmp);
    return *this;
  }

  Edge_event(Edge_event&& rhs) noexcept
    : is_owner_{rhs.is_owner_}
    , handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Edge_event& operator=(Edge_event&& rhs) noexcept
  {
    Edge_event tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Edge_event& other) noexcept
  {
    using std::swap;
    swap(is_owner_, other.is_owner_);
    swap(handle_, other.handle_);
  }

  gpiod_edge_event_type type() const noexcept
  {
    return gpiod_edge_event_get_event_type(handle_);
  }

  std::chrono::nanoseconds timestamp() const noexcept
  {
    return std::chrono::nanoseconds{gpiod_edge_event_get_timestamp_ns(handle_)};
  }

  unsigned line_offset() const noexcept
  {
    return gpiod_edge_event_get_line_offset(handle_);
  }

  unsigned long global_seqno() const noexcept
  {
    return gpiod_edge_event_get_global_seqno(handle_);
  }

  unsigned long line_seqno() const noexcept
  {
    return gpiod_edge_event_get_line_seqno(handle_);
  }

private:
  friend class Edge_event_buffer;
  bool is_owner_{};
  gpiod_edge_event* handle_{};

  explicit Edge_event(gpiod_edge_event* const handle) noexcept
    : is_owner_{false}
    , handle_{handle}
  {
    assert(handle_);
  }
};

// -----------------------------------------------------------------------------

class Edge_event_buffer final {
public:
  ~Edge_event_buffer()
  {
    if (handle_) {
      gpiod_edge_event_buffer_free(handle_);
      handle_ = nullptr;
    }
  }

  Edge_event_buffer() = default;

  explicit Edge_event_buffer(const std::size_t capacity)
    : handle_{gpiod_edge_event_buffer_new(capacity)}
  {
    if (!handle_)
      throw Exception{"cannot create edge event buffer"};
  }

  Edge_event_buffer(const Edge_event_buffer&) = delete;
  Edge_event_buffer& operator=(const Edge_event_buffer&) = delete;

  Edge_event_buffer(Edge_event_buffer&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Edge_event_buffer& operator=(Edge_event_buffer&& rhs) noexcept
  {
    Edge_event_buffer tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Edge_event_buffer& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  std::size_t capacity() const noexcept
  {
    return gpiod_edge_event_buffer_get_capacity(handle_);
  }

  std::size_t event_count() const noexcept
  {
    return gpiod_edge_event_buffer_get_num_events(handle_);
  }

  Edge_event event(const unsigned long index) const noexcept
  {
    return Edge_event{gpiod_edge_event_buffer_get_event(handle_, index)};
  }

private:
  friend class Line_request;
  gpiod_edge_event_buffer* handle_{};
};

// -----------------------------------------------------------------------------

class Chip_info final {
public:
  ~Chip_info()
  {
    if (handle_) {
      gpiod_chip_info_free(handle_);
      handle_ = nullptr;
    }
  }

  Chip_info() = default;

  Chip_info(const Chip_info& rhs) = delete;
  Chip_info& operator=(const Chip_info&) = delete;

  Chip_info(Chip_info&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Chip_info& operator=(Chip_info&& rhs) noexcept
  {
    Chip_info tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Chip_info& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  std::string name() const
  {
    return gpiod_chip_info_get_name(handle_);
  }

  std::string label() const
  {
    return gpiod_chip_info_get_label(handle_);
  }

  std::size_t line_count() const noexcept
  {
    return gpiod_chip_info_get_num_lines(handle_);
  }

private:
  friend class Chip;
  gpiod_chip_info* handle_{};

  explicit Chip_info(gpiod_chip_info* const handle) noexcept
    : handle_{handle}
  {
    assert(handle_);
  }
};

// -----------------------------------------------------------------------------

class Line_info final {
public:
  ~Line_info()
  {
    if (is_owner_ && handle_) {
      gpiod_line_info_free(handle_);
      handle_ = nullptr;
    }
  }

  Line_info() = default;

  Line_info(const Line_info& rhs)
    : is_owner_{true}
    , handle_{gpiod_line_info_copy(rhs.handle_)}
  {
    if (!handle_)
      throw Exception{"cannot copy line info object"};
  }

  Line_info& operator=(const Line_info& rhs)
  {
    Line_info tmp{rhs};
    swap(tmp);
    return *this;
  }

  Line_info(Line_info&& rhs) noexcept
    : is_owner_{rhs.is_owner_}
    , handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Line_info& operator=(Line_info&& rhs) noexcept
  {
    Line_info tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Line_info& other) noexcept
  {
    using std::swap;
    swap(is_owner_, other.is_owner_);
    swap(handle_, other.handle_);
  }

  unsigned offset() const noexcept
  {
    return gpiod_line_info_get_offset(handle_);
  }

  std::string name() const
  {
    const char* const result{gpiod_line_info_get_name(handle_)};
    return result ? result : "";
  }

  bool is_used() const noexcept
  {
    return gpiod_line_info_is_used(handle_);
  }

  std::string consumer() const
  {
    const char* const result{gpiod_line_info_get_consumer(handle_)};
    return result ? result : "";
  }

  gpiod_line_direction direction() const noexcept
  {
    return gpiod_line_info_get_direction(handle_);
  }

  gpiod_line_edge edge() const noexcept
  {
    return gpiod_line_info_get_edge_detection(handle_);
  }

  gpiod_line_bias bias() const noexcept
  {
    return gpiod_line_info_get_bias(handle_);
  }

  gpiod_line_drive drive() const noexcept
  {
    return gpiod_line_info_get_drive(handle_);
  }

  bool is_active_low() const noexcept
  {
    return gpiod_line_info_is_active_low(handle_);
  }

  bool is_debounced() const noexcept
  {
    return gpiod_line_info_is_debounced(handle_);
  }

  std::chrono::microseconds debounce_period() const noexcept
  {
    return std::chrono::microseconds{gpiod_line_info_get_debounce_period_us(handle_)};
  }

  gpiod_line_clock event_clock() const noexcept
  {
    return gpiod_line_info_get_event_clock(handle_);
  }

private:
  friend class Chip;
  friend class Info_event;
  bool is_owner_{};
  gpiod_line_info* handle_{};

  explicit Line_info(gpiod_line_info* const handle, const bool is_owner) noexcept
    : is_owner_{is_owner}
    , handle_{handle}
  {
    assert(handle_);
  }
};

// -----------------------------------------------------------------------------

class Info_event final {
public:
  ~Info_event()
  {
    if (handle_) {
      gpiod_info_event_free(handle_);
      handle_ = nullptr;
    }
  }

  Info_event() = default;

  Info_event(const Info_event&) = delete;
  Info_event& operator=(const Info_event&) = delete;

  Info_event(Info_event&& rhs) noexcept
    : Info_event{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Info_event& operator=(Info_event&& rhs) noexcept
  {
    Info_event tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Info_event& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  gpiod_info_event_type type() const noexcept
  {
    return gpiod_info_event_get_event_type(handle_);
  }

  std::chrono::nanoseconds timestamp() const noexcept
  {
    return std::chrono::nanoseconds{gpiod_info_event_get_timestamp_ns(handle_)};
  }

  Line_info line_info() const noexcept
  {
    return Line_info{gpiod_info_event_get_line_info(handle_), false};
  }

private:
  friend class Chip;
  gpiod_info_event* handle_{};

  explicit Info_event(gpiod_info_event* const handle) noexcept
    : handle_{handle}
  {
    assert(handle_);
  }
};

// -----------------------------------------------------------------------------

class Line_settings final {
public:
  ~Line_settings()
  {
    if (handle_) {
      gpiod_line_settings_free(handle_);
      handle_ = nullptr;
    }
  }

  Line_settings()
    : Line_settings{gpiod_line_settings_new()}
  {}

  Line_settings(const Line_settings& rhs)
    : handle_{gpiod_line_settings_copy(rhs.handle_)}
  {
    if (!handle_)
      throw Exception{"cannot copy line settings"};
  }

  Line_settings& operator=(const Line_settings& rhs)
  {
    Line_settings tmp{rhs};
    swap(tmp);
    return *this;
  }

  Line_settings(Line_settings&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Line_settings& operator=(Line_settings&& rhs) noexcept
  {
    Line_settings tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Line_settings& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  void reset()
  {
    gpiod_line_settings_reset(handle_);
  }

  void set_direction(const gpiod_line_direction direction)
  {
    if (gpiod_line_settings_set_direction(handle_, direction))
      throw Exception{"cannot set line direction"};
  }

  gpiod_line_direction direction() const noexcept
  {
    return gpiod_line_settings_get_direction(handle_);
  }

  void set_edge_detection(const gpiod_line_edge edge)
  {
    if (gpiod_line_settings_set_edge_detection(handle_, edge))
      throw Exception{"cannot set edge detection"};
  }

  gpiod_line_edge edge_detection() const noexcept
  {
    return gpiod_line_settings_get_edge_detection(handle_);
  }

  void set_bias(const gpiod_line_bias bias)
  {
    if (gpiod_line_settings_set_bias(handle_, bias))
      throw Exception{"cannot set bias"};
  }

  gpiod_line_bias bias() const noexcept
  {
    return gpiod_line_settings_get_bias(handle_);
  }

  void set_drive(const gpiod_line_drive drive)
  {
    if (gpiod_line_settings_set_drive(handle_, drive))
      throw Exception{"cannot set drive"};
  }

  gpiod_line_drive drive() const noexcept
  {
    return gpiod_line_settings_get_drive(handle_);
  }

  void set_active_low(const bool active)
  {
    gpiod_line_settings_set_active_low(handle_, active);
  }

  bool is_active_low() const noexcept
  {
    return gpiod_line_settings_get_active_low(handle_);
  }

  void set_debounce_period(const std::chrono::microseconds period)
  {
    gpiod_line_settings_set_debounce_period_us(handle_, period.count());
  }

  std::chrono::microseconds debounce_period() const noexcept
  {
    return std::chrono::microseconds{gpiod_line_settings_get_debounce_period_us(handle_)};
  }

  void set_event_clock(const gpiod_line_clock event_clock)
  {
    if (gpiod_line_settings_set_event_clock(handle_, event_clock))
      throw Exception{"cannot set event clock"};
  }

  gpiod_line_clock event_clock() const noexcept
  {
    return gpiod_line_settings_get_event_clock(handle_);
  }

  void set_output_value(const gpiod_line_value value)
  {
    if (gpiod_line_settings_set_output_value(handle_, value))
      throw Exception{"cannot set output value"};
  }

  gpiod_line_value output_value() const noexcept
  {
    return gpiod_line_settings_get_output_value(handle_);
  }

private:
  friend class Line_config;
  gpiod_line_settings* handle_;

  explicit Line_settings(gpiod_line_settings* const handle)
    : handle_{handle}
  {
    if (!handle_)
      throw Exception{"cannot create line settings object"};
  }
};

// -----------------------------------------------------------------------------

class Line_config final {
public:
  ~Line_config()
  {
    if (handle_) {
      gpiod_line_config_free(handle_);
      handle_ = nullptr;
    }
  }

  Line_config()
    : handle_{gpiod_line_config_new()}
  {
    if (!handle_)
      throw Exception{"cannot create line config object"};
  }

  Line_config(const Line_config&) = delete;
  Line_config& operator=(const Line_config&) = delete;

  Line_config(Line_config&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Line_config& operator=(Line_config&& rhs) noexcept
  {
    Line_config tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Line_config& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  void reset()
  {
    gpiod_line_config_reset(handle_);
  }

  void add_line_settings(const std::vector<unsigned>& offsets,
    const Line_settings& settings)
  {
    if (gpiod_line_config_add_line_settings(handle_, offsets.data(),
        offsets.size(), settings.handle_))
      throw Exception{"cannot add line settings for a set of offsets"};
  }

  Line_settings line_settings(const unsigned offset) const
  {
    auto* const result = gpiod_line_config_get_line_settings(handle_, offset);
    if (!result)
      throw Exception{"cannot get line settings for offset "
        + std::to_string(offset)};
    return Line_settings{result};
  }

  void set_output_values(const std::vector<gpiod_line_value>& values)
  {
    if (gpiod_line_config_set_output_values(handle_, values.data(), values.size()))
      throw Exception{"cannot set output values for a number of lines"};
  }

  std::size_t line_offset_count() const noexcept
  {
    return gpiod_line_config_get_num_configured_offsets(handle_);
  }

  std::vector<unsigned> offsets(const std::size_t max_size) const
  {
    std::vector<unsigned> result(max_size);
    const auto size = gpiod_line_config_get_configured_offsets(handle_,
      result.data(), result.size());
    result.resize(size);
    return result;
  }

private:
  friend class Chip;
  friend class Line_request;
  gpiod_line_config* handle_;
};

// -----------------------------------------------------------------------------

class Line_request final {
public:
  ~Line_request()
  {
    if (handle_) {
      gpiod_line_request_release(handle_);
      handle_ = nullptr;
    }
  }

  Line_request() = default;

  Line_request(const Line_request&) = delete;
  Line_request& operator=(const Line_request&) = delete;

  Line_request(Line_request&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Line_request& operator=(Line_request&& rhs) noexcept
  {
    Line_request tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Line_request& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  std::size_t line_count() const noexcept
  {
    return gpiod_line_request_get_num_requested_lines(handle_);
  }

  std::vector<unsigned> offsets(const std::size_t max_size) const
  {
    std::vector<unsigned> result(max_size);
    const auto size = gpiod_line_request_get_requested_offsets(handle_,
      result.data(), result.size());
    result.resize(size);
    return result;
  }

  gpiod_line_value value(const unsigned offset) const noexcept
  {
    return gpiod_line_request_get_value(handle_, offset);
  }

  std::vector<gpiod_line_value> values(const std::vector<unsigned>& offsets) const
  {
    std::vector<gpiod_line_value> result(offsets.size());
    const int r{gpiod_line_request_get_values_subset(handle_, offsets.size(),
        offsets.data(), result.data())};
    if (r)
      fill(result.begin(), result.end(), GPIOD_LINE_VALUE_ERROR);
    return result;
  }

  std::vector<gpiod_line_value> values() const
  {
    std::vector<gpiod_line_value> result(line_count());
    const int r{gpiod_line_request_get_values(handle_, result.data())};
    if (r)
      fill(result.begin(), result.end(), GPIOD_LINE_VALUE_ERROR);
    return result;
  }

  void set_value(const unsigned offset, const gpiod_line_value value)
  {
    if (gpiod_line_request_set_value(handle_, offset, value))
      throw Exception{"cannot set the value of a single requested line,"
        " offset " + std::to_string(offset)};
  }

  void set_values(const std::vector<unsigned>& offsets,
    const std::vector<gpiod_line_value>& values)
  {
    if (offsets.size() != values.size())
      throw Exception{"cannot set values: offsets and values not consistent"};
    if (gpiod_line_request_set_values_subset(handle_, offsets.size(),
        offsets.data(), values.data()))
      throw Exception{"cannot set the values of a subset of requested lines"};
  }

  void set_values(const std::vector<gpiod_line_value>& values)
  {
    if (gpiod_line_request_set_values(handle_, values.data()))
      throw Exception{"cannot set the values of all lines associated with a request"};
  }

  void reconfigure(const Line_config& config)
  {
    if (gpiod_line_request_reconfigure_lines(handle_, config.handle_))
      throw Exception{"cannot update the configuration of lines"
        " associated with a line request"};
  }

  int file_descriptor() const noexcept
  {
    return gpiod_line_request_get_fd(handle_);
  }

  int wait_edge_events(const std::chrono::nanoseconds timeout)
  {
    const int result{gpiod_line_request_wait_edge_events(handle_, timeout.count())};
    if (result == -1) // check for -1 is important
      throw Exception{"cannot wait for edge events on any of the requested lines"};
    return result;
  }

  std::size_t read_edge_events(Edge_event_buffer& result, const std::size_t max_count)
  {
    if (result.capacity() < max_count)
      throw Exception{"cannot get edge events: invalid max count"};

    const int result_size{gpiod_line_request_read_edge_events(handle_,
        result.handle_, max_count)};
    if (result_size == -1) // check for -1 is important
      throw Exception{"cannot read a number of edge events from a line request"};
    return result_size;
  }

private:
  friend class Chip;
  gpiod_line_request* handle_{};

  explicit Line_request(gpiod_line_request* const handle) noexcept
    : handle_{handle}
  {
    assert(handle_);
  }
};

// -----------------------------------------------------------------------------

class Request_config final {
public:
  ~Request_config()
  {
    if (handle_) {
      gpiod_request_config_free(handle_);
      handle_ = nullptr;
    }
  }

  Request_config()
    : handle_{gpiod_request_config_new()}
  {
    if (!handle_)
      throw Exception{"cannot create request config object"};
  }

  Request_config(const Request_config&) = delete;
  Request_config& operator=(const Request_config&) = delete;

  Request_config(Request_config&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Request_config& operator=(Request_config&& rhs) noexcept
  {
    Request_config tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Request_config& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  void set_consumer(const std::string& name)
  {
    gpiod_request_config_set_consumer(handle_, name.c_str());
  }

  std::string consumer() const noexcept
  {
    return gpiod_request_config_get_consumer(handle_);
  }

  void set_event_buffer_size(const std::size_t size)
  {
    gpiod_request_config_set_event_buffer_size(handle_, size);
  }

  std::size_t event_buffer_size() const noexcept
  {
    return gpiod_request_config_get_event_buffer_size(handle_);
  }

private:
  friend class Chip;
  gpiod_request_config* handle_;
};

// -----------------------------------------------------------------------------

class Chip final {
public:
  ~Chip()
  {
    if (handle_) {
      gpiod_chip_close(handle_);
      handle_ = nullptr;
    }
  }

  Chip() = default;

  explicit Chip(const std::filesystem::path& path)
    : handle_{gpiod_chip_open(path.string().c_str())}
  {
    if (!handle_)
      throw Exception{"cannot open gpiochip device file"};
  }

  Chip(const Chip&) = delete;
  Chip& operator=(const Chip&) = delete;

  Chip(Chip&& rhs) noexcept
    : handle_{rhs.handle_}
  {
    rhs.handle_ = nullptr;
  }

  Chip& operator=(Chip&& rhs) noexcept
  {
    Chip tmp{std::move(rhs)};
    swap(tmp);
    return *this;
  }

  void swap(Chip& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  std::filesystem::path path() const
  {
    return gpiod_chip_get_path(handle_);
  }

  Chip_info chip_info() const
  {
    if (auto* const info = gpiod_chip_get_info(handle_); !info)
      throw Exception{"cannot get information about the chip"};
    else
      return Chip_info{info};
  }

  Line_info line_info(const unsigned offset) const
  {
    if (auto* const info = gpiod_chip_get_line_info(handle_, offset); !info)
      throw Exception{"cannot get a snapshot of information about a line"};
    else
      return Line_info{info, true};
  }

  Line_info watch_line_info(const unsigned offset)
  {
    if (auto* const info = gpiod_chip_watch_line_info(handle_, offset); !info)
      throw Exception{"cannot get a snapshot of the status of a line and"
        " start watching it for future changes"};
    else
      return Line_info{info, true};
  }

  void unwatch_line_info(const unsigned offset)
  {
    if (gpiod_chip_unwatch_line_info(handle_, offset))
      throw Exception{"cannot stop watching a line for status changes"};
  }

  int file_descriptor() const noexcept
  {
    return gpiod_chip_get_fd(handle_);
  }

  int wait_info_event(const std::chrono::nanoseconds timeout)
  {
    const int result{gpiod_chip_wait_info_event(handle_, timeout.count())};
    if (result == -1) // check for -1 is important
      throw Exception{"cannot wait info event"};
    return result;
  }

  Info_event info_event() const
  {
    if (auto* const result = gpiod_chip_read_info_event(handle_); !result)
      throw Exception{"cannot read a single line status change event from chip"};
    else
      return Info_event{result};
  }

  unsigned line_offset_from_name(const std::string& name) const
  {
    const int result{gpiod_chip_get_line_offset_from_name(handle_, name.c_str())};
    if (result == -1) // check for -1 is important
      throw Exception{"cannot map a line's name \""+name+"\" to its"
        " offset within the chip"};
    return result;
  }

  Line_request line_request(const Request_config& request_config,
    const Line_config& line_config) const
  {
    return line_request(request_config.handle_, line_config.handle_);
  }

  Line_request line_request(const Line_config& line_config) const
  {
    return line_request(nullptr, line_config.handle_);
  }

private:
  gpiod_chip* handle_{};

  Line_request line_request(gpiod_request_config* const req_cfg,
    gpiod_line_config* const line_cfg) const
  {
    auto* const result = gpiod_chip_request_lines(handle_, req_cfg, line_cfg);
    if (!result)
      throw Exception{"cannot request a set of lines for exclusive usage"};
    return Line_request{result};
  }
};

// -----------------------------------------------------------------------------

inline bool is_gpiochip_device(const std::filesystem::path& path)
{
  return gpiod_is_gpiochip_device(path.string().c_str());
}

} // namespace dmitigr::gpio

#endif  // DMITIGR_GPIO_HPP
