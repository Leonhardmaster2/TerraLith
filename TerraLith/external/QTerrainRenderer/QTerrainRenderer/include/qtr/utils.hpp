/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

namespace qtr
{

template <typename T>
inline void json_safe_get(const nlohmann::json &j, const char *key, T &value)
{
  if (j.contains(key))
    value = j.at(key).get<T>();
}

std::vector<uint16_t> load_png_as_16bit_rgba(const std::string &path,
                                             int               &width,
                                             int               &height);

std::vector<uint16_t> load_png_as_16bit_grayscale(const std::string &path,
                                                  int               &width,
                                                  int               &height);

std::vector<uint8_t> load_png_as_8bit_rgb(const std::string &path,
                                          int               &width,
                                          int               &height);

std::vector<uint8_t> load_png_as_8bit_rgba(const std::string &path,
                                           int               &width,
                                           int               &height);

std::vector<float> load_png_as_grayscale(const std::string &path,
                                         int               &width,
                                         int               &height);

} // namespace qtr

// --- Specialized serialization

namespace nlohmann
{

template <> struct adl_serializer<glm::vec2>
{
  static void to_json(json &j, const glm::vec2 &value)
  {
    j = json({{"x", value.x}, {"y", value.y}});
  }

  static void from_json(const json &j, glm::vec2 &value)
  {
    j.at("x").get_to(value.x);
    j.at("y").get_to(value.y);
  }
};

template <> struct adl_serializer<glm::vec3>
{
  static void to_json(json &j, const glm::vec3 &value)
  {
    j = json({{"x", value.x}, {"y", value.y}, {"z", value.z}});
  }

  static void from_json(const json &j, glm::vec3 &value)
  {
    j.at("x").get_to(value.x);
    j.at("y").get_to(value.y);
    j.at("z").get_to(value.z);
  }
};

} // namespace nlohmann