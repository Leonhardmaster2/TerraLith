/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include <stdexcept>

#include <QOpenGLFunctions>

#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "qtr/config.hpp"
#include "qtr/imgui_widgets.hpp"
#include "qtr/logger.hpp"
#include "qtr/mesh.hpp"
#include "qtr/primitives.hpp"
#include "qtr/render_widget.hpp"
#include "qtr/utils.hpp"

namespace qtr
{

void RenderWidget::render_scene_render_2d()
{
  // model
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::scale(model, glm::vec3(1.f, this->scale_h, 1.f));
  model = glm::translate(
      model,
      glm::vec3(this->viewer2d_settings.offset.x, 0.f, this->viewer2d_settings.offset.y));

  // projection
  float aspect_ratio = static_cast<float>(this->width()) /
                       static_cast<float>(this->height());

  // --- main lit pass

  this->setup_gl_state();

  QOpenGLShaderProgram *p_shader = this->sp_shader_manager->get("viewer2d_cmap")->get();

  if (p_shader)
  {
    p_shader->bind();

    this->set_common_uniforms(*p_shader,
                              model,
                              glm::mat4(0.f),
                              glm::mat4(0.f),
                              glm::mat4(0.f));

    p_shader->setUniformValue("use_texture_albedo", true);
    p_shader->setUniformValue("normal_visualization", false);

    p_shader->setUniformValue("aspect_ratio", aspect_ratio);
    p_shader->setUniformValue("zoom", this->viewer2d_settings.zoom);
    p_shader->setUniformValue("sun_azimuth", this->viewer2d_settings.sun_azimuth);
    p_shader->setUniformValue("sun_zenith", this->viewer2d_settings.sun_zenith);
    p_shader->setUniformValue("hillshading", this->viewer2d_settings.hillshading);
    p_shader->setUniformValue("cmap", this->viewer2d_settings.cmap);

    // heightmap
    if (this->render_hmap)
    {
      p_shader->setUniformValue("base_color", QVector3D(0.8f, 0.8f, 0.8f));
      p_shader->setUniformValue(
          "use_texture_albedo",
          true && !this->bypass_texture_albedo &&
              this->sp_texture_manager->get(QTR_TEX_ALBEDO)->is_active());

      // if (this->sp_texture_manager->get(QTR_TEX_NORMAL)->is_active())
      //   p_shader->setUniformValue("normal_map_scaling", this->normal_map_scaling);

      p_shader->setUniformValue("normal_map_scaling", 0.f);
      p_shader->setUniformValue("use_texture_albedo", false);

      this->hmap.draw();
    }

    this->unbind_textures();

    p_shader->release();
  }
}

void RenderWidget::render_ui_render_2d()
{
  ImGui::SetCurrentContext(this->imgui_context);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();

  // --- Overlay: FPS ---
  bool changed = false;

  changed |= imgui_viewer_main_menubar(*this);

  ImGui::SetNextWindowBgAlpha(0.95f);
  ImGui::Begin("Render settings");

  // --- View & Camera ---
  ImGui::SeparatorText("View");

  if (ImGui::Button("Reset view"))
  {
    this->viewer2d_settings.zoom = 0.8f;
    this->viewer2d_settings.offset = glm::vec2(0.f, 0.f);

    this->need_update = true;
  }

  std::vector<std::string> cmap_labels = {"Gray", "Viridis", "Turbo", "Magma"};

  int cmap_int = static_cast<int>(this->viewer2d_settings.cmap);
  if (imgui_enum_selector("Colormap", cmap_int, cmap_labels))
  {
    this->viewer2d_settings.cmap = static_cast<Viewer2DSettings::Colormap>(cmap_int);
    this->need_update = true;
  }

  // --- Lighting ---
  if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
  {
    changed |= ImGui::SliderAngle("Azimuth",
                                  &this->viewer2d_settings.sun_azimuth,
                                  -180.f,
                                  180.f);
    changed |= ImGui::SliderAngle("Zenith",
                                  &this->viewer2d_settings.sun_zenith,
                                  0.f,
                                  90.f);
    changed |= ImGui::Checkbox("Hillshading", &this->viewer2d_settings.hillshading);
  }

  // --- End main window ---
  this->need_update |= changed;
  ImGui::End();

  // --- IO / camera control ---
  ImGuiIO &io = this->get_imgui_io();

  if (!io.WantCaptureMouse) // only outside the ImGUI window
  {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
      this->viewer2d_settings.offset.x += io.MouseDelta.x * 0.002f /
                                          this->viewer2d_settings.zoom;
      this->viewer2d_settings.offset.y -= io.MouseDelta.y * 0.002f /
                                          this->viewer2d_settings.zoom;
    }

    if (io.MouseWheel != 0.0f)
    {
      this->viewer2d_settings.zoom *= (1.0f + io.MouseWheel * 0.1f);
      this->viewer2d_settings.zoom = std::max(this->viewer2d_settings.zoom, 0.f);
    }
  }
  else
  {
    // to force frame update for ImGUI submenus...
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
      this->need_update = true;
  }

  // --- Render ---
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace qtr
