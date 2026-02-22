/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <random>

#include <glm/gtc/constants.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

void generate_rock(Mesh &mesh,
                   float radius,
                   float roughness,
                   uint  seed,
                   int   subdivisions = 1)
{
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;

  // ---- Icosahedron base ----
  const float            t = (1.0 + sqrt(5.0)) / 2.0;
  std::vector<glm::vec3> base_positions = {{-1, t, 0},
                                           {1, t, 0},
                                           {-1, -t, 0},
                                           {1, -t, 0},
                                           {0, -1, t},
                                           {0, 1, t},
                                           {0, -1, -t},
                                           {0, 1, -t},
                                           {t, 0, -1},
                                           {t, 0, 1},
                                           {-t, 0, -1},
                                           {-t, 0, 1}};
  std::vector<uint> base_indices = {0, 11, 5, 0, 5,  1,  0,  1,  7,  0,  7, 10, 0, 10, 11,
                                    1, 5,  9, 5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1,  8,
                                    3, 9,  4, 3, 4,  2,  3,  2,  6,  3,  6, 8,  3, 8,  9,
                                    4, 9,  5, 2, 4,  11, 6,  2,  10, 8,  6, 7,  9, 8,  1};

  // Optional: subdivide (simple midpoint subdivision)
  auto midpoint = [&](glm::vec3 a, glm::vec3 b)
  { return glm::normalize((a + b) * 0.5f); };

  std::vector<glm::vec3> positions = base_positions;
  std::vector<uint>      tris = base_indices;

  for (int s = 0; s < subdivisions; ++s)
  {
    std::map<std::pair<uint, uint>, uint> midpoint_cache;

    auto get_midpoint = [&](uint i1, uint i2) -> uint
    {
      if (i1 > i2)
        std::swap(i1, i2);
      auto key = std::make_pair(i1, i2);
      if (midpoint_cache.count(key))
        return midpoint_cache[key];
      glm::vec3 mpos = midpoint(positions[i1], positions[i2]);
      positions.push_back(mpos);
      uint idx = static_cast<uint>(positions.size() - 1);
      midpoint_cache[key] = idx;
      return idx;
    };

    std::vector<uint> new_tris;
    for (size_t i = 0; i < tris.size(); i += 3)
    {
      uint i0 = tris[i], i1 = tris[i + 1], i2 = tris[i + 2];
      uint a = get_midpoint(i0, i1);
      uint b = get_midpoint(i1, i2);
      uint c = get_midpoint(i2, i0);

      new_tris.insert(new_tris.end(), {i0, a, c});
      new_tris.insert(new_tris.end(), {i1, b, a});
      new_tris.insert(new_tris.end(), {i2, c, b});
      new_tris.insert(new_tris.end(), {a, b, c});
    }
    tris.swap(new_tris);
  }

  // ---- Random displacement ----
  std::mt19937                          gen(seed);
  std::uniform_real_distribution<float> dist(-roughness, roughness);

  vertices.reserve(positions.size());
  for (auto &p : positions)
  {
    glm::vec3 displaced = glm::normalize(p) * radius;
    displaced += glm::normalize(p) * dist(gen) * radius;

    vertices.emplace_back(displaced,
                          glm::normalize(p), // placeholder, recalc later
                          glm::vec2(0.0f));  // no UVs
  }

  indices = tris;

  // ---- Recalculate normals ----
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    Vertex &v0 = vertices[indices[i + 0]];
    Vertex &v1 = vertices[indices[i + 1]];
    Vertex &v2 = vertices[indices[i + 2]];

    glm::vec3 edge1 = v1.position - v0.position;
    glm::vec3 edge2 = v2.position - v0.position;
    glm::vec3 n = glm::normalize(glm::cross(edge1, edge2));

    v0.normal += n;
    v1.normal += n;
    v2.normal += n;
  }
  for (auto &v : vertices)
    v.normal = glm::normalize(v.normal);

  mesh.create(vertices, indices);
}

} // namespace qtr
