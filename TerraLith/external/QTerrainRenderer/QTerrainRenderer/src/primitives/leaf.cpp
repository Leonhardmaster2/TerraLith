/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <random>

#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_grass_leaf_2sided(Mesh            &mesh,
                                const glm::vec3 &base_pos,
                                float            height,
                                float            width,
                                float            bend = 0.2f)
{
  std::vector<Vertex>       vertices;
  std::vector<unsigned int> indices;

  // --- Define vertices ---

  glm::vec3 tip = base_pos + glm::vec3(0.f, height, bend * height);  // top point
  glm::vec3 bottomL = base_pos + glm::vec3(-width * 0.5f, 0.f, 0.f); // bottom left
  glm::vec3 bottomR = base_pos + glm::vec3(width * 0.5f, 0.f, 0.f);  // bottom right

  glm::vec2 uv_tip(0.5f, 1.f);
  glm::vec2 uv_bl(0.f, 0.f);
  glm::vec2 uv_br(1.f, 0.f);

  glm::vec3 normal(0.f, 1.f, 0.f); // approximate

  // front face
  vertices.emplace_back(bottomL, normal, uv_bl);
  vertices.emplace_back(bottomR, normal, uv_br);
  vertices.emplace_back(tip, normal, uv_tip);

  // back face (flip normal)
  vertices.emplace_back(bottomL, -normal, uv_bl);
  vertices.emplace_back(bottomR, -normal, uv_br);
  vertices.emplace_back(tip, -normal, uv_tip);

  // --- Indices ---

  // front triangle
  indices.insert(indices.end(), {0, 1, 2});
  // back triangle (reversed winding)
  indices.insert(indices.end(), {3, 5, 4});

  mesh.create(vertices, indices);
}

} // namespace qtr
