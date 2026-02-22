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

void RenderWidget::render_scene_render_3d()
{
  if (QOpenGLContext::currentContext() != this->context())
    this->makeCurrent();

  // model
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::scale(model, glm::vec3(1.f, this->scale_h, 1.f));

  // shadow map
  glm::mat4 light_space_matrix;
  this->render_shadow_map(model, light_space_matrix);

  // projection - guard against zero height
  int h = this->height();
  int w = this->width();
  if (h <= 0 || w <= 0)
    return;

  float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);

  glm::mat4 projection = this->camera.get_projection_matrix_perspective(aspect_ratio);

  // depth map
  this->render_depth_map(model, this->camera.get_view_matrix(), projection);

  // --- main lit pass

  this->setup_gl_state();

  QOpenGLShaderProgram *p_shader = this->sp_shader_manager->get("shadow_map_lit_pass")
                                       ->get();

  if (p_shader)
  {
    p_shader->bind();

    this->set_common_uniforms(*p_shader,
                              model,
                              projection,
                              this->camera.get_view_matrix(),
                              light_space_matrix);

    // base plane
    if (this->render_plane)
    {
      p_shader->setUniformValue("base_color", QVector3D(0.2f, 0.2f, 0.2f));
      p_shader->setUniformValue("add_ambiant_occlusion", false);
      this->plane.draw();
    }

    // points
    if (this->render_points)
    {
      p_shader->setUniformValue("add_ambiant_occlusion", false);
      this->points_instanced_mesh.draw(p_shader);
    }

    // path
    if (this->render_path)
    {
      p_shader->setUniformValue("base_color", QVector3D(1.f, 0.f, 1.f));
      p_shader->setUniformValue("add_ambiant_occlusion", false);
      this->path_mesh.draw();
    }

    // heightmap
    if (this->render_hmap)
    {
      p_shader->setUniformValue("base_color", QVector3D(1.f, 1.f, 1.f));
      p_shader->setUniformValue(
          "use_texture_albedo",
          true && !this->bypass_texture_albedo &&
              this->sp_texture_manager->get(QTR_TEX_ALBEDO)->is_active());

      if (this->sp_texture_manager->get(QTR_TEX_NORMAL)->is_active())
        p_shader->setUniformValue("normal_map_scaling", this->normal_map_scaling);

      p_shader->setUniformValue("add_ambiant_occlusion", this->add_ambiant_occlusion);
      this->hmap.draw();

      p_shader->setUniformValue("normal_map_scaling", 0.f);
      p_shader->setUniformValue("use_texture_albedo", false);
    }

    if (this->render_rocks)
      this->rocks_instanced_mesh.draw(p_shader);

    if (this->render_leaves)
      this->leaves_instanced_mesh.draw(p_shader);

    if (this->render_trees)
      this->trees_instanced_mesh.draw(p_shader);

    if (this->render_water)
    {
      p_shader->setUniformValue("spec_strength", this->water_spec_strength);

      p_shader->setUniformValue("add_ambiant_occlusion", false);
      p_shader->setUniformValue("use_texture_albedo", false);

      p_shader->setUniformValue("use_water_colors", true);

      p_shader->setUniformValue("color_shallow_water", toQVec(this->color_shallow_water));
      p_shader->setUniformValue("color_deep_water", toQVec(this->color_deep_water));

      p_shader->setUniformValue("water_color_depth", this->water_color_depth);

      p_shader->setUniformValue("add_water_foam", this->add_water_foam);
      p_shader->setUniformValue("foam_color", toQVec(this->foam_color));
      p_shader->setUniformValue("foam_depth", this->foam_depth);

      p_shader->setUniformValue("add_water_waves", this->add_water_waves);
      p_shader->setUniformValue("angle_spread_ratio", this->angle_spread_ratio);
      p_shader->setUniformValue("waves_alpha", this->waves_alpha);
      p_shader->setUniformValue("waves_kw", this->waves_kw);
      p_shader->setUniformValue("waves_amplitude", this->waves_amplitude);
      p_shader->setUniformValue("waves_normal_amplitude", this->waves_normal_amplitude);

      if (this->animate_waves)
        p_shader->setUniformValue("waves_speed", this->waves_speed);
      else
        p_shader->setUniformValue("waves_speed", 0.f);

      // either use the input mesh or use a simple plane surface as a
      // fallback
      if (this->water_mesh.is_active())
        this->water_mesh.draw();
    }

    this->unbind_textures();

    p_shader->release();
  }
}

void RenderWidget::render_ui_render_3d()
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
  changed |= ImGui::Checkbox("Normal visualization", &this->normal_visualization);
  ImGui::SameLine();
  changed |= ImGui::Checkbox("Wireframe", &this->wireframe_mode);
  changed |= ImGui::SliderFloat("Height scale", &this->scale_h, 0.f, 2.f);
  changed |= ImGui::SliderAngle("FOV", &this->camera.fov, 10.f, 180.f);
  changed |= ImGui::Checkbox("Auto rotate cam.", &this->auto_rotate_camera);

  // Mesh resolution
  {
    const char *mesh_res_labels[] = {"Full", "1/2", "1/4", "1/8"};
    if (ImGui::Combo("Mesh resolution",
                     &this->mesh_downsample_level,
                     mesh_res_labels,
                     IM_ARRAYSIZE(mesh_res_labels)))
    {
      changed = true;
    }
  }

  if (ImGui::Button("Reset Camera"))
  {
    this->reset_camera_position();
    this->need_update = true;
  }

  // --- Rendering Toggles ---
  ImGui::SeparatorText("Render Options");

  if (ImGui::BeginTable("#CheckGrid", 2))
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    changed |= ImGui::Checkbox("Plane", &this->render_plane);
    //
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    changed |= ImGui::Checkbox("Terrain", &this->render_hmap);
    ImGui::TableNextColumn();
    changed |= ImGui::Checkbox("Water##render", &this->render_water);
    //
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    changed |= ImGui::Checkbox("Points", &this->render_points);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    changed |= ImGui::Checkbox("Path", &this->render_path);
    //
    // ImGui::TableNextRow();
    // ImGui::TableNextColumn();
    // changed |= ImGui::Checkbox("Rocks", &this->render_rocks);
    // ImGui::TableNextColumn();
    // changed |= ImGui::Checkbox("Trees", &this->render_trees);

    ImGui::EndTable();
  }

  // --- Materials ---
  if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Albedo");
    changed |= ImGui::SliderFloat("Gamma", &this->gamma_correction, 0.01f, 4.f);
    changed |= ImGui::Checkbox("Bypass albedo", &this->bypass_texture_albedo);
    changed |= ImGui::Checkbox("Tonemap", &this->apply_tonemap);

    ImGui::Text("Normal Map");
    changed |= ImGui::SliderFloat("Scaling", &this->normal_map_scaling, 0.f, 2.f);
  }

  // --- Lighting ---
  if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
  {
    changed |= ImGui::SliderAngle("Azimuth", &this->light_phi, -180.f, 180.f);
    changed |= ImGui::SliderAngle("Zenith", &this->light_theta, 0.f, 90.f);
    changed |= ImGui::Checkbox("Auto rotate", &this->auto_rotate_light);

    ImGui::Text("Shadow Map");
    changed |= ImGui::Checkbox("Bypass", &this->bypass_shadow_map);
    changed |= ImGui::SliderFloat("Strength", &this->shadow_strength, 0.f, 1.f);

    {
      const int   resolutions[] = {512, 1024, 2048, 4096, 8192};
      const char *labels[] = {"512", "1024", "2048", "4096", "8192"};
      int         current_idx = 2; // default to 2048
      for (int i = 0; i < 5; ++i)
        if (resolutions[i] == this->shadow_map_resolution)
          current_idx = i;

      if (ImGui::Combo("Shadow Res", &current_idx, labels, 5))
      {
        this->set_shadow_map_resolution(resolutions[current_idx]);
        changed = true;
      }
    }

    if (ImGui::TreeNode("Ambient Occlusion"))
    {
      changed |= ImGui::Checkbox("Enable AO", &this->add_ambiant_occlusion);
      changed |= ImGui::SliderFloat("Strength",
                                    &this->ambiant_occlusion_strength,
                                    0.f,
                                    1.f);
      changed |= ImGui::SliderFloat("Radius", &this->ambiant_occlusion_radius, 0.f, 0.5f);
      ImGui::TreePop();
    }
  }

  // --- Water ---
  if (ImGui::CollapsingHeader("Water", ImGuiTreeNodeFlags_DefaultOpen))
  {
    changed |= ImGui::SliderFloat("Color depth", &this->water_color_depth, 0.f, 0.2f);
    changed |= ImGui::SliderFloat("Specularity", &this->water_spec_strength, 0.f, 1.f);
    changed |= imgui_show_water_preset_selector(this->color_shallow_water,
                                                this->color_deep_water);

    ImGui::Separator();

    changed |= ImGui::Checkbox("Foam", &this->add_water_foam);
    if (this->add_water_foam)
      changed |= ImGui::SliderFloat("Foam depth", &this->foam_depth, 0.f, 0.1f);

    changed |= ImGui::Checkbox("Waves", &this->add_water_waves);
    if (this->add_water_waves)
    {
      changed |= ImGui::SliderFloat("Wavenumber", &this->waves_kw, 0.f, 2048.f);
      changed |= ImGui::SliderFloat("Amplitude", &this->waves_amplitude, 0.f, 0.1f);
      changed |= ImGui::SliderFloat("Normal amplitude",
                                    &this->waves_normal_amplitude,
                                    0.f,
                                    0.1f);
      changed |= ImGui::SliderAngle("Angle", &this->waves_alpha, -180.f, 180.f);
      changed |= ImGui::SliderFloat("Angle spread", &this->angle_spread_ratio, 0.f, 0.1f);
      changed |= ImGui::Checkbox("Animate", &this->animate_waves);
      if (this->animate_waves)
        changed |= ImGui::SliderFloat("Speed", &this->waves_speed, 0.f, 1.f);
    }
  }

  // --- Atmosphere ---
  if (ImGui::CollapsingHeader("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen))
  {
    changed |= ImGui::Checkbox("Fog", &this->add_fog);
    changed |= ImGui::SliderFloat("Density##fog", &this->fog_density, 0.f, 100.f);
    changed |= ImGui::SliderFloat("Height##fog", &this->fog_height, 0.f, 1.f);
    changed |= ImGui::ColorEdit3("Color##fog", glm::value_ptr(this->fog_color));

    changed |= ImGui::Checkbox("Scattering", &this->add_atmospheric_scattering);
    changed |= ImGui::SliderFloat("Density##scat", &this->scattering_density, 0.f, 1.f);
    changed |= ImGui::SliderFloat("Fog strength##scat", &this->fog_strength, 0.f, 1.f);
    changed |= ImGui::SliderFloat("Scattering ratio##scat",
                                  &this->fog_scattering_ratio,
                                  0.f,
                                  1.f);
    changed |= ImGui::ColorEdit3("Rayleigh color", glm::value_ptr(this->rayleigh_color));
    changed |= ImGui::ColorEdit3("Mie color", glm::value_ptr(this->mie_color));
  }

  // --- Mouse controls overlay ---
  if (QTR_CONFIG->viewer3d.show_mouse_control)
  {
    // Offset from top-right corner
    const ImVec2 padding(20.0f, 20.0f);

    // Get main viewport
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    // Position at top-right
    ImVec2 pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - padding.x,
                        viewport->WorkPos.y + padding.y);

    // Window flags for overlay
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    // Remove frame/border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    // Set exact screen position
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("MouseControlsOverlay", nullptr, flags))
    {
      ImGui::Text("LMB: Rotate");
      ImGui::Text("Wheel: Zoom");
      ImGui::Text("RMB: Pan");
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
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
      this->alpha_y -= io.MouseDelta.x * 0.005f;
      this->alpha_x += io.MouseDelta.y * 0.005f;
      this->alpha_x = glm::clamp(this->alpha_x,
                                 -0.99f * glm::half_pi<float>(),
                                 0.99f * glm::half_pi<float>());
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
      this->pan_offset.x -= io.MouseDelta.x * 0.001f * this->distance;
      this->pan_offset.y += io.MouseDelta.y * 0.001f * this->distance;
    }

    if (io.MouseWheel != 0.0f)
    {
      this->distance *= (1.0f - io.MouseWheel * 0.1f);
      this->distance = glm::clamp(this->distance, 0.f, 50.0f);
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
