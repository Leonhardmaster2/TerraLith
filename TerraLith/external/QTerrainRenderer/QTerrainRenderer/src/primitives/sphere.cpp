/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <random>

#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_sphere(Mesh &mesh, float radius, int slices, int stacks)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  vertices.reserve((stacks + 1) * (slices + 1));

  for (int j = 0; j <= stacks; ++j)
  {
    float v = static_cast<float>(j) / stacks;
    float phi = v * glm::pi<float>(); // latitude [0, pi]

    for (int i = 0; i <= slices; ++i)
    {
      float u = static_cast<float>(i) / slices;
      float theta = u * glm::two_pi<float>(); // longitude [0, 2pi]

      float x = sin(phi) * cos(theta);
      float y = cos(phi);
      float z = sin(phi) * sin(theta);

      glm::vec3 normal(x, y, z);
      glm::vec3 position = radius * normal;

      vertices.emplace_back(position, normal, glm::vec2(u, v));
    }
  }

  for (int j = 0; j < stacks; ++j)
    for (int i = 0; i < slices; ++i)
    {
      int row1 = j * (slices + 1);
      int row2 = (j + 1) * (slices + 1);

      int i0 = row1 + i;
      int i1 = row1 + i + 1;
      int i2 = row2 + i;
      int i3 = row2 + i + 1;

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
