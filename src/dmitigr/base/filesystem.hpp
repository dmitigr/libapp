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

#include <locale>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <vector>

namespace dmitigr::filesystem {

/**
 * @returns The vector of the paths.
 *
 * @param root - the search root;
 * @param extension - the extension of files to be included into the result;
 * @param recursive - if `true` then do the recursive search;
 * @param include_heading - if `true` then include the "heading file" (see
 * remarks) into the result.
 *
 * @remarks The "heading file" - is a regular file with the given `extension`
 * which has the same parent directory as the `root`.
 */
inline std::vector<std::filesystem::path>
file_paths_by_extension(const std::filesystem::path& root,
  const std::filesystem::path& extension,
  const bool recursive, const bool include_heading = false)
{
  std::vector<std::filesystem::path> result;

  if (is_regular_file(root) && root.extension() == extension)
    return {root};

  if (include_heading) {
    auto heading_file = root;
    heading_file.replace_extension(extension);
    if (is_regular_file(heading_file))
      result.push_back(heading_file);
  }

  if (is_directory(root)) {
    const auto traverse = [&](auto iterator)
    {
      for (const auto& dirent : iterator) {
        const auto& path = dirent.path();
        if (is_regular_file(path) && path.extension() == extension)
          result.push_back(dirent);
      }
    };

    if (recursive)
      traverse(std::filesystem::recursive_directory_iterator{root});
    else
      traverse(std::filesystem::directory_iterator{root});
  }
  return result;
}

/**
 * @brief Searches for the `dir` directory starting from `path` up to the root.
 *
 * @returns A first path to a directory in which `dir` directory found, or
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
 * @brief Uppercases the root name of `path`.
 *
 * @remarks Useful on Windows.
 */
inline std::filesystem::path to_uppercase_root_name(const std::filesystem::path& path,
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
