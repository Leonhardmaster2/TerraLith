/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <memory>
#include <vector>

#include <QOpenGLFunctions_3_3_Core>

#include <glm/glm.hpp>

namespace qtr
{

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

class Mesh : protected QOpenGLFunctions_3_3_Core
{
public:
  Mesh();
  ~Mesh();

  void create(std::vector<Vertex> vertices,
              std::vector<uint>   indices = {},
              bool                store_cpu_copy = false,
              std::vector<int>    vertex_map = {});

  void                 destroy();
  void                 draw();
  size_t               get_index_count() const;
  std::vector<uint>   &get_indices();
  GLuint               get_vao() const;
  std::vector<int>    &get_vertex_map();
  std::vector<Vertex> &get_vertices();
  bool                 is_active() const;
  void                 update_vertices(const std::vector<Vertex> &vertices);
  void                 update_vertices();

private:
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  size_t vertex_count = 0;
  size_t index_count = 0;
  bool   has_indices;

  // storage (optional)
  std::vector<Vertex> vertices;
  std::vector<uint>   indices;
  std::vector<int>    vertex_map;
};

} // namespace qtr
