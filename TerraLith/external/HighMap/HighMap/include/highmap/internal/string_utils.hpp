/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file string_utils.hpp
 * @author Otto Link (otto.link.bv@gmail.com)
 * @brief
 *
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <filesystem>
#include <string>

namespace hmap
{

/**
 * @brief Adds a suffix to the filename of a given file path.
 *
 * This function appends a given suffix to the stem (base name without
 * extension) of a file while preserving the original directory and file
 * extension.
 *
 * @param  file_path The original file path.
 * @param  suffix    The suffix to append to the filename.
 * @return           A new std::filesystem::path with the modified filename.
 *
 * @note If the input file has no extension, the suffix is added directly to the
 * filename.
 *
 * @example
 * @code std::filesystem::path path = "example.txt";
 * std::filesystem::path new_path = add_filename_suffix(path, "_backup");
 * std::cout << new_path; // Outputs "example_backup.txt"
 * @endcode
 */
std::filesystem::path add_filename_suffix(
    const std::filesystem::path &file_path,
    const std::string           &suffix);

/**
 * @brief Pads the beginning of a string with zeros until it reaches a specified
 * length.
 *
 * This function prepends '0' characters to the input string `str` so that the
 * resulting string has at least `n_zero` characters. If `str` is already equal
 * to or longer than `n_zero`, it is returned unchanged.
 *
 * @param  str    The input string to pad.
 * @param  n_zero The minimum total length of the resulting string after
 *                padding.
 * @return        A new string padded with leading zeros to reach the specified
 *                length.
 *
 * @example zfill("42", 5); // returns "00042"
 * zfill("12345", 5); // returns "12345"
 */
std::string zfill(const std::string &str, int n_zero);

} // namespace hmap