/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_rectangle_mesh(Mesh            &mesh,
                             const glm::vec3 &p1,
                             const glm::vec3 &p2,
                             float            height)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  // Direction from one edge center to opposite edge center
  glm::vec3 width_dir = glm::normalize(p2 - p1);

  // Find perpendicular direction for height
  glm::vec3 up;
  if (fabs(width_dir.y) < 0.99f)
    up = glm::normalize(glm::cross(width_dir, glm::vec3(0, 1, 0)));
  else
    up = glm::normalize(glm::cross(width_dir, glm::vec3(1, 0, 0)));

  glm::vec3 offset = up * (height * 0.5f);

  // Generate 4 vertices
  glm::vec3 v0 = p1 - offset;
  glm::vec3 v1 = p1 + offset;
  glm::vec3 v2 = p2 + offset;
  glm::vec3 v3 = p2 - offset;

  // Simple upward normal (works if rectangle lies horizontally)
  glm::vec3 normal = glm::normalize(glm::cross(width_dir, up));

  // UVs for basic mapping
  glm::vec2 uv0(0.0f, 0.0f);
  glm::vec2 uv1(0.0f, 1.0f);
  glm::vec2 uv2(1.0f, 1.0f);
  glm::vec2 uv3(1.0f, 0.0f);

  vertices.emplace_back(v0, normal, uv0);
  vertices.emplace_back(v1, normal, uv1);
  vertices.emplace_back(v2, normal, uv2);
  vertices.emplace_back(v3, normal, uv3);

  // Two triangles forming the rectangle
  indices = {0, 1, 2, 0, 2, 3};

  // Build the mesh
  mesh.create(vertices, indices);
}

} // namespace qtr
