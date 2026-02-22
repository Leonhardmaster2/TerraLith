/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

namespace qtr
{

struct Light
{
  glm::vec3 position;
  glm::vec3 target = glm::vec3(0.f);

  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  glm::vec3 get_dir() const;
  void      set_position_spherical(float distance, float theta, float phi);
};

} // namespace qtr