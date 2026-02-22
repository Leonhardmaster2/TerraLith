/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_plane(Mesh &mesh, float x, float y, float z, float lx, float ly)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  float hx = lx * 0.5f; // half size along X
  float hz = ly * 0.5f; // half size along Z

  glm::vec3 normal(0.0f, 1.0f, 0.0f); // up direction

  // 4 vertices (counter-clockwise)
  vertices = {
      {glm::vec3(x - hx, y, z - hz), normal, glm::vec2(0.0f, 0.0f)}, // bottom-left
      {glm::vec3(x + hx, y, z - hz), normal, glm::vec2(1.0f, 0.0f)}, // bottom-right
      {glm::vec3(x + hx, y, z + hz), normal, glm::vec2(1.0f, 1.0f)}, // top-right
      {glm::vec3(x - hx, y, z + hz), normal, glm::vec2(0.0f, 1.0f)}  // top-left
  };

  // indices for two triangles
  indices = {0, 1, 2, 2, 3, 0};

  mesh.create(vertices, indices);
}

} // namespace qtr
