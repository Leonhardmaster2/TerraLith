/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include "qtr/render_widget.hpp"

namespace qtr
{

void RenderWidget::render_shadow_map(const glm::mat4 &model,
                                     glm::mat4       &light_space_matrix)
{
  // shadow depth pass, camera at the light position
  this->camera_shadow_pass.position = this->light.position;
  this->camera_shadow_pass.near_plane = 0.f;
  this->camera_shadow_pass.far_plane = 100.f;

  float     ortho_size = 1.5f; // TODO hardcoded
  glm::mat4 light_projection = this->camera_shadow_pass.get_projection_matrix_ortho(
      ortho_size);
  glm::mat4 light_view = this->camera_shadow_pass.get_view_matrix();
  light_space_matrix = light_projection * light_view;

  QOpenGLShaderProgram *p_shader = this->sp_shader_manager->get("shadow_map_depth_pass")
                                       ->get();

  if (p_shader)
  {
    Texture *p_tex = this->sp_texture_manager->get(QTR_TEX_SHADOW_MAP);

    // backup FBO state to avoid messing up with others FBO (ImGUI
    // for instance...)
    GLint previous_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);

    glViewport(0, 0, p_tex->get_width(), p_tex->get_height());
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    p_shader->bind();
    p_shader->setUniformValue("light_space_matrix", toQMat(light_space_matrix));
    p_shader->setUniformValue("model", toQMat(model));

    if (this->render_plane)
      this->plane.draw();

    if (this->render_hmap)
      this->hmap.draw();

    // no water

    if (this->render_rocks)
      this->rocks_instanced_mesh.draw(p_shader);

    if (this->render_leaves)
      this->leaves_instanced_mesh.draw(p_shader);

    if (this->render_trees)
      this->trees_instanced_mesh.draw(p_shader);

    p_shader->release();

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // set previous FBO back
    glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);
  }
}

} // namespace qtr
