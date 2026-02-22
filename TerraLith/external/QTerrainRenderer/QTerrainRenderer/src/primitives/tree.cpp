/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_tree(Mesh &mesh,
                   float trunk_height,
                   float trunk_radius,
                   float crown_height,
                   float crown_radius,
                   int   trunk_segments)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  // -------------------
  // Trunk (cylinder sides only)
  // -------------------
  int   base_index = 0;
  float angle_step = glm::two_pi<float>() / trunk_segments;

  for (int i = 0; i <= trunk_segments; ++i)
  {
    float     angle = i * angle_step;
    float     cx = cos(angle);
    float     cz = sin(angle);
    glm::vec3 normal(cx, 0, cz);

    glm::vec3 bottom(cx * trunk_radius, 0.0f, cz * trunk_radius);
    glm::vec3 top(cx * trunk_radius, trunk_height, cz * trunk_radius);

    vertices.emplace_back(bottom, normal, glm::vec2(i / float(trunk_segments), 0.0f));
    vertices.emplace_back(top, normal, glm::vec2(i / float(trunk_segments), 1.0f));
  }

  for (int i = 0; i < trunk_segments; ++i)
  {
    int i0 = base_index + i * 2;
    int i1 = base_index + i * 2 + 1;
    int i2 = base_index + (i + 1) * 2;
    int i3 = base_index + (i + 1) * 2 + 1;

    indices.push_back(i0);
    indices.push_back(i2);
    indices.push_back(i1);

    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
  }

  // -------------------
  // Crown (cone)
  // -------------------
  int       crown_base = static_cast<int>(vertices.size());
  glm::vec3 tip(0, trunk_height + crown_height, 0);

  for (int i = 0; i <= trunk_segments; ++i)
  {
    float angle = i * angle_step;
    float cx = cos(angle);
    float cz = sin(angle);

    glm::vec3 base_pos(cx * crown_radius, trunk_height, cz * crown_radius);
    glm::vec3 dir = glm::normalize(glm::vec3(cx, crown_height / crown_radius, cz));

    vertices.emplace_back(base_pos, dir, glm::vec2(i / float(trunk_segments), 0.0f));
    vertices.emplace_back(tip, dir, glm::vec2(i / float(trunk_segments), 1.0f));
  }

  for (int i = 0; i < trunk_segments; ++i)
  {
    int i0 = crown_base + i * 2;
    int i1 = crown_base + i * 2 + 1;
    int i2 = crown_base + (i + 1) * 2;
    int i3 = crown_base + (i + 1) * 2 + 1;

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
