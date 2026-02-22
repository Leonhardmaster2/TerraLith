/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include "qtr/render_widget.hpp"

namespace qtr
{

void RenderWidget::render_depth_map(const glm::mat4 &model,
                                    const glm::mat4 &view,
                                    const glm::mat4 &projection)
{
  QOpenGLShaderProgram *p_shader = this->sp_shader_manager->get("depth_map")->get();

  if (p_shader)
  {
    Texture *p_tex = this->sp_texture_manager->get(QTR_TEX_DEPTH);

    // backup FBO state to avoid messing up with others FBO (ImGUI
    // for instance...)
    GLint previous_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);

    glViewport(0, 0, p_tex->get_width(), p_tex->get_height());
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_depth);

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    p_shader->bind();
    p_shader->setUniformValue("model", toQMat(model));
    p_shader->setUniformValue("view", toQMat(view));
    p_shader->setUniformValue("projection", toQMat(projection));

    if (this->render_plane)
      this->plane.draw();

    if (this->render_hmap)
      this->hmap.draw();

    if (this->render_water)
      this->water_mesh.draw();

    if (this->render_leaves)
      this->leaves_instanced_mesh.draw(p_shader);

    if (this->render_rocks)
      this->rocks_instanced_mesh.draw(p_shader);

    if (this->render_trees)
      this->trees_instanced_mesh.draw(p_shader);

    p_shader->release();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // set previous FBO back
    glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);
  }
}

} // namespace qtr
