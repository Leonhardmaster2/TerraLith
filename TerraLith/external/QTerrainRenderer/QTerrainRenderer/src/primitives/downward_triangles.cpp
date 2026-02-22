/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_downward_triangles(Mesh                         &mesh,
                                 const std::vector<glm::vec3> &points,
                                 float                         height_offset,
                                 float                         radius)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  vertices.reserve(points.size() * 4);
  indices.reserve(points.size() * 6);

  for (size_t i = 0; i < points.size(); ++i)
  {
    const glm::vec3 &tip = points[i];

    // Tip of triangle (at reference point)
    glm::vec3 apex = tip;

    // Base center is slightly above along +Y
    glm::vec3 base_center = tip + glm::vec3(0.0f, height_offset, 0.0f);

    // Base vertices in XZ plane rotated at 120° intervals
    glm::vec3 v0 = base_center + glm::vec3(radius, 0.0f, 0.0f);
    glm::vec3 v1 = base_center +
                   glm::vec3(radius * -0.5f, 0.0f, radius * glm::sqrt(3.0f) / 2.0f);
    glm::vec3 v2 = base_center +
                   glm::vec3(radius * -0.5f, 0.0f, radius * -glm::sqrt(3.0f) / 2.0f);

    uint start_index = static_cast<uint>(vertices.size());

    // Add vertices (temporary normals = up)
    vertices.push_back({apex, glm::vec3(0, 1, 0), glm::vec2(0.5f, 1.0f)});
    vertices.push_back({v0, glm::vec3(0, 1, 0), glm::vec2(0.0f, 0.0f)});
    vertices.push_back({v1, glm::vec3(0, 1, 0), glm::vec2(1.0f, 0.0f)});
    vertices.push_back({v2, glm::vec3(0, 1, 0), glm::vec2(0.5f, 0.0f)});

    // Side faces
    indices.push_back(start_index + 0);
    indices.push_back(start_index + 1);
    indices.push_back(start_index + 2);

    indices.push_back(start_index + 0);
    indices.push_back(start_index + 2);
    indices.push_back(start_index + 3);

    // Base cap (optional, to close bottom)
    indices.push_back(start_index + 1);
    indices.push_back(start_index + 3);
    indices.push_back(start_index + 2);
  }

  // Recompute normals
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    Vertex &v0 = vertices[indices[i + 0]];
    Vertex &v1 = vertices[indices[i + 1]];
    Vertex &v2 = vertices[indices[i + 2]];

    glm::vec3 edge1 = v1.position - v0.position;
    glm::vec3 edge2 = v2.position - v0.position;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    v0.normal += normal;
    v1.normal += normal;
    v2.normal += normal;
  }

  for (auto &v : vertices)
    v.normal = glm::normalize(v.normal);

  // Create mesh
  mesh.create(vertices, indices);
}

} // namespace qtr
