/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>

#include <QApplication>
#include <QColor>

#include "highmap/array.hpp"
#include "highmap/geometry/cloud.hpp"
#include "highmap/geometry/path.hpp"
#include "highmap/heightmap.hpp"

#include "nlohmann/json.hpp"

namespace hesiod
{

struct StyleSettings
{
  StyleSettings() = default;

  // --- Serialization
  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  // --- Data
  // TerraLith semantic port colors
  std::map<std::string, QColor> data_color_map = {
      {typeid(hmap::Array).name(), QColor(213, 146, 53, 255)},            // Mask/Amber #D59235
      {typeid(hmap::Cloud).name(), QColor(135, 206, 235, 255)},           // Cloud/SkyBlue #87CEEB
      {typeid(hmap::Heightmap).name(), QColor(67, 150, 178, 255)},        // Heightmap/Teal #4396B2
      {typeid(std::vector<hmap::Heightmap>).name(), QColor(255, 85, 85, 255)}, // HeightmapVec/Red
      {typeid(hmap::HeightmapRGBA).name(), QColor(80, 200, 120, 255)},    // Texture/Green #50C878
      {typeid(hmap::Path).name(), QColor(255, 127, 127, 255)},            // Path/Coral #FF7F7F
      {typeid(std::vector<float>).name(), QColor(160, 160, 160, 255)},    // Scalar/Grey #A0A0A0
  };

  std::map<std::string, QColor> category_color_map = {
      {"Converter", QColor(188, 182, 163, 255)},
      {"Comment", QColor(170, 170, 170, 255)},
      {"Debug", QColor(200, 0, 0, 255)},
      {"Math", QColor(0, 43, 54, 255)},
      {"Geometry", QColor(101, 123, 131, 255)},
      {"Roads", QColor(147, 161, 161, 255)},
      {"Routing", QColor(188, 182, 163, 255)},
      {"IO", QColor(203, 196, 177, 255)},
      {"Features", QColor(181, 137, 0, 255)},
      {"Erosion", QColor(203, 75, 22, 255)},
      {"Mask", QColor(211, 54, 130, 255)},
      {"Filter", QColor(108, 113, 196, 255)},
      {"Operator", QColor(108, 113, 196, 255)},
      {"Hydrology", QColor(38, 139, 210, 255)},
      {"Primitive", QColor(42, 161, 152, 255)},
      {"Biomes", QColor(133, 153, 0, 255)},
      {"Texture", QColor(0, 0, 0, 255)},
      {"WIP", QColor(255, 255, 255, 255)},
  };
};

} // namespace hesiod