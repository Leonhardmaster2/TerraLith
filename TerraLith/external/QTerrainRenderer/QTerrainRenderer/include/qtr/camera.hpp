/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

namespace qtr
{

struct Camera
{
  glm::vec3 position;
  glm::vec3 target = glm::vec3(0.f);
  glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
  float     fov = 45.f / 180.f * 3.14f;
  float     near_plane = 0.01f;
  float     far_plane = 100.f;

  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  glm::mat4 get_projection_matrix_ortho(float ortho_size) const;
  glm::mat4 get_projection_matrix_perspective(float aspect_ratio) const;
  glm::mat4 get_view_matrix() const;
  void      set_position_angles(float distance, float alpha_x, float alpha_y);
};

} // namespace qtr