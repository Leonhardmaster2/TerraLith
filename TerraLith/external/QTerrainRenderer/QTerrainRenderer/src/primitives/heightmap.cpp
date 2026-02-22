/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <glm/gtc/constants.hpp>

#include "qtr/logger.hpp"
#include "qtr/mesh.hpp"

namespace qtr
{

void generate_heightmap(Mesh                     &mesh,
                        const std::vector<float> &data,
                        int                       width,
                        int                       height,
                        float                     x,
                        float                     y,
                        float                     z,
                        float                     lx,
                        float                     ly,
                        float                     lz,
                        bool                      add_skirt,
                        float                     add_level,
                        float                     exclude_below,
                        float                    *p_hmin)
{
  const int           count = width * height;
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;
  std::vector<int>    vertex_map(count, -1);

  vertices.reserve(count);
  indices.reserve((width - 1) * (height - 1) * 6);

  const float hx = lx * 0.5f;
  const float hz = lz * 0.5f;
  const float dx = lx / (width - 1);
  const float dz = lz / (height - 1);

  // ---- find hmin + build vertices in same pass ----
  float hmin = std::numeric_limits<float>::max();

  for (int j = 0; j < height; ++j)
  {
    const float zpos = z - hz + j * dz;
    for (int i = 0; i < width; ++i)
    {
      int   idx = j * width + i;
      float hraw = data[idx];

      if (hraw <= exclude_below)
        continue;

      // track minimum
      if (hraw < hmin)
        hmin = hraw;

      float ypos = y + hraw * ly + add_level;
      float xpos = x - hx + i * dx;

      int new_index = (int)vertices.size();
      vertex_map[idx] = new_index;

      glm::vec3 pos(xpos, ypos, zpos);
      glm::vec2 uv((float)i / (width - 1), (float)j / (height - 1));

      vertices.emplace_back(pos, glm::vec3(0, 0, 0), uv);
    }
  }

  if (p_hmin)
    *p_hmin = hmin;

  // ---- indices generation ----
  for (int j = 0; j < height - 1; ++j)
  {
    const int row0 = j * width;
    const int row1 = row0 + width;

    for (int i = 0; i < width - 1; ++i)
    {
      int i0 = row0 + i;
      int i1 = i0 + 1;
      int i2 = row1 + i;
      int i3 = i2 + 1;

      int v0 = vertex_map[i0];
      int v1 = vertex_map[i1];
      int v2 = vertex_map[i2];
      int v3 = vertex_map[i3];

      if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0)
        continue;

      // tri 1
      indices.push_back(v0);
      indices.push_back(v2);
      indices.push_back(v1);

      // tri 2
      indices.push_back(v1);
      indices.push_back(v2);
      indices.push_back(v3);
    }
  }

  // ---- skirts ----
  if (add_skirt)
  {
    float skirt_y = y + hmin * ly;

    auto add_skirt_edge = [&](auto index_of, int count)
    {
      for (int k = 0; k < count - 1; ++k)
      {
        int top_a = vertex_map[index_of(k)];
        int top_b = vertex_map[index_of(k + 1)];
        if (top_a < 0 || top_b < 0)
          continue;

        int base = (int)vertices.size();

        glm::vec3 a = vertices[top_a].position;
        a.y = skirt_y;
        glm::vec3 b = vertices[top_b].position;
        b.y = skirt_y;

        vertices.emplace_back(a, glm::vec3(0), vertices[top_a].uv);
        vertices.emplace_back(b, glm::vec3(0), vertices[top_b].uv);

        int bot_a = base;
        int bot_b = base + 1;

        indices.push_back(top_a);
        indices.push_back(bot_a);
        indices.push_back(top_b);

        indices.push_back(top_b);
        indices.push_back(bot_a);
        indices.push_back(bot_b);
      }
    };

    add_skirt_edge([&](int j) { return j * width; }, height);
    add_skirt_edge([&](int j) { return j * width + (width - 1); }, height);
    add_skirt_edge([&](int i) { return i; }, width);
    add_skirt_edge([&](int i) { return (height - 1) * width + i; }, width);
  }

  // ---- normals ----
  for (size_t k = 0; k < indices.size(); k += 3)
  {
    auto &v0 = vertices[indices[k + 0]];
    auto &v1 = vertices[indices[k + 1]];
    auto &v2 = vertices[indices[k + 2]];

    glm::vec3 n = glm::normalize(
        glm::cross(v1.position - v0.position, v2.position - v0.position));
    v0.normal += n;
    v1.normal += n;
    v2.normal += n;
  }
  for (auto &v : vertices)
    v.normal = glm::normalize(v.normal);

  mesh.create(std::move(vertices),
              std::move(indices),
              /* store_cpu_copy */ true,
              std::move(vertex_map));
}

void update_heightmap_elevation(Mesh                     &mesh,
                                const std::vector<float> &data,
                                int                       width,
                                int                       height,
                                float                     y,
                                float                     ly,
                                float                    &hmin,
                                float                     add_level)
{
  auto &verts = mesh.get_vertices();
  auto &inds = mesh.get_indices();
  auto &vertex_map = mesh.get_vertex_map();

  hmin = std::numeric_limits<float>::max();

  // vertices beyond that index are skirt vertices
  int max_index = 0;

  // update Y positions
  for (int j = 0; j < height; ++j)
    for (int i = 0; i < width; ++i)
    {
      int idx = j * width + i;
      int vindex = vertex_map[idx]; // store this once at creation

      max_index = std::max(max_index, vindex);

      if (vindex < 0)
        continue;

      float hraw = data[idx];
      hmin = std::min(hmin, hraw);

      verts[vindex].position.y = y + hraw * ly + add_level;
    }

  // fix skirt elevation
  if (max_index + 1 < verts.size())
  {
    for (size_t k = max_index + 1; k < verts.size(); ++k)
      verts[k].position.y = y + hmin * ly + add_level;
  }

  // reset normals to zero before re-accumulation
  for (auto &v : verts)
    v.normal = glm::vec3(0.f);

  // normals
  for (size_t k = 0; k < inds.size(); k += 3)
  {
    auto &v0 = verts[inds[k]];
    auto &v1 = verts[inds[k + 1]];
    auto &v2 = verts[inds[k + 2]];

    glm::vec3 n = glm::normalize(
        glm::cross(v1.position - v0.position, v2.position - v0.position));

    v0.normal += n;
    v1.normal += n;
    v2.normal += n;
  }

  for (auto &v : verts)
    v.normal = glm::normalize(v.normal);

  mesh.update_vertices();
}

} // namespace qtr
