/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/matrix_transform.hpp>

#include "qtr/camera.hpp"
#include "qtr/logger.hpp"
#include "qtr/utils.hpp"

namespace qtr
{

glm::mat4 Camera::get_projection_matrix_ortho(float ortho_size) const
{
  return glm::ortho(-ortho_size,
                    ortho_size,
                    -ortho_size,
                    ortho_size,
                    this->near_plane,
                    this->far_plane);
}

glm::mat4 Camera::get_projection_matrix_perspective(float aspect_ratio) const
{
  return glm::perspective(this->fov, aspect_ratio, this->near_plane, this->far_plane);
}

glm::mat4 Camera::get_view_matrix() const
{
  return glm::lookAt(this->position, this->target, this->up);
}

void Camera::json_from(const nlohmann::json &json)
{
  json_safe_get(json, "position", this->position);
  json_safe_get(json, "target", this->target);
  json_safe_get(json, "up", this->up);
  json_safe_get(json, "fov", this->fov);
  json_safe_get(json, "near_plane", this->near_plane);
  json_safe_get(json, "far_plane", this->far_plane);
}

nlohmann::json Camera::json_to() const
{
  return {{"position", this->position},
          {"target", this->target},
          {"up", this->up},
          {"fov", this->fov},
          {"near_plane", this->near_plane},
          {"far_plane", this->far_plane}};
}

void Camera::set_position_angles(float distance, float alpha_x, float alpha_y)
{
  this->position.x = distance * std::cos(alpha_x) * std::sin(alpha_y);
  this->position.y = distance * std::sin(alpha_x);
  this->position.z = distance * std::cos(alpha_x) * std::cos(alpha_y);
}

} // namespace qtr
