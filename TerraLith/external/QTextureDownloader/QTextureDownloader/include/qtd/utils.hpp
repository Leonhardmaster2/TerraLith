/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>

#include <QImage>

#include "nlohmann/json.hpp"

namespace qtd
{

template <typename T> bool contains(const std::vector<T> &vec, const T &item)
{
  return std::find(vec.begin(), vec.end(), item) != vec.end();
}

nlohmann::json json_from_file(const std::string &fname);
void           json_to_file(const nlohmann::json &json,
                            const std::string    &fname,
                            bool                  merge_with_existing_content = false);

bool download_file(const std::string &url,
                   const std::string &file_path,
                   bool               overwrite = false);

} // namespace qtd