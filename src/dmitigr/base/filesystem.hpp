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

#ifndef DMITIGR_BASE_FILESYSTEM_HPP
#define DMITIGR_BASE_FILESYSTEM_HPP

#if (defined(__clang__) && (__clang_major__ < 7)) || \
    (defined(__GNUG__)  && (__GNUC__ < 8) && !defined (__clang__))
  #include <experimental/filesystem>
  namespace std {
  namespace filesystem = experimental::filesystem;
  } // namespace std
#else
  #include <filesystem>
#endif

#include <fstream>
#include <locale>
#include <optional>
#include <stdexcept>
#include <vector>

namespace dmitigr::filesystem {

/**
 * @brief Calls the `callback` for each file under the `root`.
 *
 * @param callback A function with signature
 * `bool callback(const std::filesystem::path&)`.
 * @param root A root directory.
 * @param recursive A recursive traversal indicator.
 */
template<typename F>
void for_each(F&& callback,
  const std::filesystem::path& root,
  const bool recursive = false)
{
  if (is_directory(root)) {
    const auto traverse = [&callback](const auto& iterator)
    {
      for (const auto& dirent : iterator) {
        if (!callback(dirent))
          break;
      }
    };
    if (recursive)
      traverse(std::filesystem::recursive_directory_iterator{root});
    else
      traverse(std::filesystem::directory_iterator{root});
  }
}

/**
 * @brief Calls the `callback` for each regular file under the `root`.
 *
 * @details If the `root` represents the regular file, it's the only value
 * passed to the `callback`.
 *
 * @param callback A function with signature
 * `bool callback(const std::filesystem::path&)`.
 * @param root A search root.
 * @param recursive A recursive traversal indicator.
 */
template<typename F>
void for_each_regular_file(F&& callback,
  const std::filesystem::path& root,
  const bool recursive = false)
{
  if (is_regular_file(root)) {
    (void)callback(root);
    return;
  }

  for_each([&callback](const std::filesystem::path& path)
  {
    if (is_regular_file(path))
      return callback(path);
    return true;
  }, root, recursive);
}

/**
 * @brief Searches for the `child` directory starting from `path` up to the root.
 *
 * @returns A first path to a directory in which `child` directory found, or
 * `std::nullopt` if no such a directory found.
 */
inline std::optional<std::filesystem::path>
first_parent(std::filesystem::path path, const std::filesystem::path& child)
{
  while (true) {
    if (exists(path / child))
      return path;
    else if (path.has_parent_path())
      path = path.parent_path();
    else
      return std::nullopt;
  }
}

/**
 * @returns If there is a directory `root` and a regular file `root.extension`
 * under the directory `root.parent_path()`, the returned value is the path to
 * such a file.
 */
inline std::filesystem::path heading_file(const std::filesystem::path& root,
  const std::filesystem::path& extension)
{
  if (is_directory(root)) {
    auto result = root;
    result.replace_extension(extension);
    if (is_regular_file(result))
      return result;
  }
  return std::filesystem::path{};
}

/**
 * @brief Uppercases the root name of `path`.
 *
 * @remarks Useful on Windows.
 */
inline std::filesystem::path
to_uppercase_root_name(const std::filesystem::path& path,
  const std::locale& loc = {})
{
  if (!path.has_root_name())
    return path;

  auto result = path.root_name();
  auto root_name = result.wstring();
  const auto& facet = std::use_facet<std::ctype<wchar_t>>(loc);
  facet.toupper(root_name.data(), root_name.data() + root_name.size());
  result = std::filesystem::path{root_name};
  result += path.root_directory();
  return result += path.relative_path();
}

/// Creates the file at `path` and writes the `data` into it.
inline void overwrite(const std::filesystem::path& path,
  const std::string_view data)
{
  if (!data.data())
    throw std::invalid_argument{"cannot overwrite file "+path.string()+":"
      " invalid data"};
  else if (std::ofstream pf{path, std::ios_base::trunc})
    pf.write(data.data(), data.size());
  else
    throw std::runtime_error{"cannot overwrite file "+path.string()+":"
      " cannot open file"};
}

} // namespace dmitigr::filesystem

#endif  // DMITIGR_BASE_FILESYSTEM_HPP
