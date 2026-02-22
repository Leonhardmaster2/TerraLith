/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <stdexcept>
#include <vector>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

#include <glm/glm.hpp>

#include "qtr/mesh.hpp"

namespace qtr
{

// Base instancing template
template <typename T> class InstancedMesh : protected QOpenGLFunctions_3_3_Core
{
public:
  InstancedMesh() : instance_vbo(0), instance_count(0) {}

  void create(std::shared_ptr<Mesh> sp_base_mesh, const std::vector<T> &instances)
  {
    QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
    this->destroy();

    this->sp_mesh = sp_base_mesh;
    this->instance_count = static_cast<int>(instances.size());

    glBindVertexArray(this->sp_mesh->get_vao());

    // Upload instance data
    glGenBuffers(1, &this->instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->instance_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 instances.size() * sizeof(T),
                 instances.data(),
                 GL_STATIC_DRAW);

    // Tell OpenGL how to read T
    setup_attributes();

    glBindVertexArray(0);
  }

  void draw(QOpenGLShaderProgram *p_shader)
  {
    if (!p_shader || !this->is_active())
      return;

    p_shader->setUniformValue("has_instances", true);

    glBindVertexArray(this->sp_mesh->get_vao());
    glDrawElementsInstanced(GL_TRIANGLES,
                            this->sp_mesh->get_index_count(),
                            GL_UNSIGNED_INT,
                            nullptr,
                            this->instance_count);
    glBindVertexArray(0);

    p_shader->setUniformValue("has_instances", false);
  }

  void destroy()
  {
    if (this->instance_vbo)
      glDeleteBuffers(1, &this->instance_vbo);

    if (this->sp_mesh)
      this->sp_mesh->destroy();

    this->instance_vbo = 0;
    this->instance_count = 0;
  }

  bool is_active()
  {
    bool state = this->sp_mesh ? this->sp_mesh->is_active() : false;
    return state && (this->instance_vbo);
  }

private:
  std::shared_ptr<Mesh> sp_mesh;
  GLuint                instance_vbo;
  int                   instance_count;

  // must be specialized for each Instance type
  void setup_attributes()
  {
    throw std::runtime_error(
        "InstancedMesh::setup_attributes: not defined for this instanced mesh");
  }
};

// --- Basic translate/scale/rotate/color instance ---

struct BaseInstance
{
  glm::vec3 position;
  float     scale;
  float     rotation;
  glm::vec3 color;
};

// --- Specialize setup ---

template <> inline void InstancedMesh<BaseInstance>::setup_attributes()
{
  GLsizei stride = sizeof(BaseInstance);

  // position
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        stride,
                        (void *)offsetof(BaseInstance, position));
  glVertexAttribDivisor(3, 1);

  // scale
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4,
                        1,
                        GL_FLOAT,
                        GL_FALSE,
                        stride,
                        (void *)offsetof(BaseInstance, scale));
  glVertexAttribDivisor(4, 1);

  // rotation
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5,
                        1,
                        GL_FLOAT,
                        GL_FALSE,
                        stride,
                        (void *)offsetof(BaseInstance, rotation));
  glVertexAttribDivisor(5, 1);

  // color
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        stride,
                        (void *)offsetof(BaseInstance, color));
  glVertexAttribDivisor(6, 1);
}

} // namespace qtr