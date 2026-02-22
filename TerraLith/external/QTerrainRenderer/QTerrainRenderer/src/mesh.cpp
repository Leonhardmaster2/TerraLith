/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/mesh.hpp"
#include "qtr/logger.hpp"

namespace qtr
{

Mesh::Mesh() {}

Mesh::~Mesh() { this->destroy(); }

void Mesh::create(std::vector<Vertex> vertices_in,
                  std::vector<uint>   indices_in,
                  bool                store_cpu_copy,
                  std::vector<int>    vertex_map_in)
{
  this->initializeOpenGLFunctions();
  this->destroy();

  this->vertex_count = vertices_in.size();
  this->index_count = indices_in.size();
  this->has_indices = !indices_in.empty();

  // Create VAO
  glGenVertexArrays(1, &this->vao);
  glBindVertexArray(this->vao);

  // Create VBO
  glGenBuffers(1, &this->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  glBufferData(GL_ARRAY_BUFFER,
               vertices_in.size() * sizeof(Vertex),
               vertices_in.data(),
               GL_DYNAMIC_DRAW);

  // Create EBO
  if (this->has_indices)
  {
    glGenBuffers(1, &this->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices_in.size() * sizeof(uint),
                 indices_in.data(),
                 GL_STATIC_DRAW);
  }
  else
  {
    this->ebo = 0;
  }

  // Vertex attributes
  GLsizei stride = sizeof(Vertex);
  glEnableVertexAttribArray(0); // position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);

  glEnableVertexAttribArray(1); // normal
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));

  glEnableVertexAttribArray(2); // textcoord
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));

  glBindVertexArray(0);

  if (store_cpu_copy)
  {
    this->vertices = std::move(vertices_in);
    this->indices = std::move(indices_in);
    this->vertex_map = std::move(vertex_map_in);
  }
}

void Mesh::draw()
{
  if (!this->vao)
    return;

  glBindVertexArray(this->vao);
  if (this->has_indices)
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(this->index_count),
                   GL_UNSIGNED_INT,
                   nullptr);
  else
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(this->vertex_count));
  glBindVertexArray(0);
}

void Mesh::destroy()
{
  if (this->vbo)
    glDeleteBuffers(1, &this->vbo);
  if (this->ebo)
    glDeleteBuffers(1, &this->ebo);
  if (this->vao)
    glDeleteVertexArrays(1, &this->vao);

  this->vbo = 0;
  this->ebo = 0;
  this->vao = 0;

  this->vertex_count = 0;
  this->index_count = 0;
  this->has_indices = false;
}

size_t Mesh::get_index_count() const { return this->index_count; }

std::vector<uint> &Mesh::get_indices() { return this->indices; }

GLuint Mesh::get_vao() const { return this->vao; }

std::vector<Vertex> &Mesh::get_vertices() { return this->vertices; }

std::vector<int> &Mesh::get_vertex_map() { return this->vertex_map; }

bool Mesh::is_active() const { return (this->vbo && this->vao); }

void Mesh::update_vertices(const std::vector<Vertex> &vertices)
{
  if (!this->vbo)
    return;
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::update_vertices()
{
  if (!this->vbo)
    return;
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  glBufferSubData(GL_ARRAY_BUFFER,
                  0,
                  this->vertices.size() * sizeof(Vertex),
                  this->vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace qtr
