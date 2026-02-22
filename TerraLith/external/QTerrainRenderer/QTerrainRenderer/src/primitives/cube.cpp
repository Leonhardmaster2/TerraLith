/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_cube(Mesh &mesh, float x, float y, float z, float lx, float ly, float lz)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  float hx = lx * 0.5f;
  float hy = ly * 0.5f;
  float hz = lz * 0.5f;

  glm::vec3 normals[6] = {
      {0.0f, 0.0f, 1.0f},  // Front
      {0.0f, 0.0f, -1.0f}, // Back
      {0.0f, 1.0f, 0.0f},  // Top
      {0.0f, -1.0f, 0.0f}, // Bottom
      {-1.0f, 0.0f, 0.0f}, // Left
      {1.0f, 0.0f, 0.0f}   // Right
  };

  glm::vec3 positions[8] = {
      {x - hx, y - hy, z - hz}, // 0: bottom-back-left
      {x + hx, y - hy, z - hz}, // 1: bottom-back-right
      {x + hx, y + hy, z - hz}, // 2: top-back-right
      {x - hx, y + hy, z - hz}, // 3: top-back-left
      {x - hx, y - hy, z + hz}, // 4: bottom-front-left
      {x + hx, y - hy, z + hz}, // 5: bottom-front-right
      {x + hx, y + hy, z + hz}, // 6: top-front-right
      {x - hx, y + hy, z + hz}  // 7: top-front-left
  };

  glm::vec2 uvs[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

  // Faces: each face has 4 vertices (quad)
  auto addFace = [&](int i0, int i1, int i2, int i3, const glm::vec3 &normal)
  {
    uint startIndex = vertices.size();
    vertices.emplace_back(positions[i0], normal, uvs[0]);
    vertices.emplace_back(positions[i1], normal, uvs[1]);
    vertices.emplace_back(positions[i2], normal, uvs[2]);
    vertices.emplace_back(positions[i3], normal, uvs[3]);

    // two triangles per face
    indices.push_back(startIndex);
    indices.push_back(startIndex + 1);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex + 3);
  };

  // Build all 6 faces (front, back, top, bottom, left, right)
  addFace(4, 5, 6, 7, normals[0]); // Front
  addFace(1, 0, 3, 2, normals[1]); // Back
  addFace(3, 7, 6, 2, normals[2]); // Top
  addFace(0, 1, 5, 4, normals[3]); // Bottom
  addFace(0, 4, 7, 3, normals[4]); // Left
  addFace(5, 1, 2, 6, normals[5]); // Right

  mesh.create(vertices, indices);
}

} // namespace qtr
