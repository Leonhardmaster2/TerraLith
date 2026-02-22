/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_path(Mesh &mesh, const std::vector<glm::vec3> &points, float width)
{
  if (points.size() < 2)
    return; // need at least 2 points

  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  // Reserve space (2 verts per point)
  vertices.reserve(points.size() * 2);
  indices.reserve((points.size() - 1) * 6);

  // Path normal (we assume mostly XZ plane, so up = Y)
  glm::vec3 up(0, 1, 0);

  for (size_t i = 0; i < points.size(); ++i)
  {
    glm::vec3 tangent;
    if (i == 0)
      tangent = glm::normalize(points[1] - points[0]);
    else if (i == points.size() - 1)
      tangent = glm::normalize(points[i] - points[i - 1]);
    else
    {
      glm::vec3 t1 = glm::normalize(points[i] - points[i - 1]);
      glm::vec3 t2 = glm::normalize(points[i + 1] - points[i]);
      tangent = glm::normalize(t1 + t2); // smooth tangent
    }

    // Perpendicular direction (side vector)
    glm::vec3 side = glm::normalize(glm::cross(up, tangent)) * (width * 0.5f);

    glm::vec3 left = points[i] - side;
    glm::vec3 right = points[i] + side;

    glm::vec3 normal = glm::normalize(glm::cross(tangent, side));

    float u = static_cast<float>(i) / (points.size() - 1);

    vertices.emplace_back(left, normal, glm::vec2(u, 0.0f));
    vertices.emplace_back(right, normal, glm::vec2(u, 1.0f));
  }

  // Build indices for triangle strip (as quads)
  for (size_t i = 0; i < points.size() - 1; ++i)
  {
    uint i0 = i * 2;
    uint i1 = i * 2 + 1;
    uint i2 = (i + 1) * 2;
    uint i3 = (i + 1) * 2 + 1;

    indices.push_back(i0);
    indices.push_back(i2);
    indices.push_back(i1);

    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
  }

  mesh.create(vertices, indices);
}

} // namespace qtr
