/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include <stdexcept>

#include <QOpenGLFunctions>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
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

RenderWidget::RenderWidget(const std::string &_title, QWidget *parent)
    : QOpenGLWidget(parent), title(_title)
{
  qtr::Logger::log()->trace("RenderWidget::RenderWidget");

  this->setWindowTitle(this->title.c_str());
  this->setFocusPolicy(Qt::StrongFocus);
  this->setMouseTracking(true);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // force 60 FPS update
  this->connect(&frame_timer,
                &QTimer::timeout,
                [this]()
                {
                  if (this->need_update)
                  {
                    this->update();
                    this->need_update = false;
                  }
                });
  this->frame_timer.start(16);

  // init.
  this->timer.start(); // global timer
  this->reset_camera_position();

  // managers
  this->sp_shader_manager = std::make_unique<ShaderManager>();
  this->sp_texture_manager = std::make_unique<TextureManager>();

  // add placeholder for each texture
  const std::vector<std::string> tex_names = {QTR_TEX_ALBEDO,
                                              QTR_TEX_HMAP,
                                              QTR_TEX_NORMAL,
                                              QTR_TEX_SHADOW_MAP,
                                              QTR_TEX_DEPTH};
  for (auto &s : tex_names)
    this->sp_texture_manager->add(s);
}

RenderWidget::~RenderWidget()
{
  if (this->context())
  {
    // don't try to make context current in destructor Qt might have
    // already destroyed the context
    ImGui::SetCurrentContext(this->imgui_context);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext(this->imgui_context);
    this->imgui_context = nullptr;
  }
}

void RenderWidget::clear()
{
  qtr::Logger::log()->trace("RenderWidget::clear");

  this->makeCurrent();

  this->reset_heightmap_geometry();
  this->reset_water_geometry();
  this->reset_points();
  this->reset_path();
  this->reset_rocks();
  this->reset_trees();

  this->reset_textures();

  this->need_update = true;

  this->doneCurrent();
}

ImGuiIO &RenderWidget::get_imgui_io()
{
  ImGui::SetCurrentContext(this->imgui_context);
  return ImGui::GetIO();
}

bool RenderWidget::get_bypass_texture_albedo() const
{
  return this->bypass_texture_albedo;
}

bool RenderWidget::get_render_plane() const { return this->render_plane; }

bool RenderWidget::get_render_points() const { return this->render_points; }

bool RenderWidget::get_render_path() const { return this->render_path; }

bool RenderWidget::get_render_hmap() const { return this->render_hmap; }

bool RenderWidget::get_render_rocks() const { return this->render_rocks; }

bool RenderWidget::get_render_trees() const { return this->render_trees; }

bool RenderWidget::get_render_water() const { return this->render_water; }

bool RenderWidget::get_render_leaves() const { return this->render_leaves; }

Mesh &RenderWidget::get_water_mesh() { return this->water_mesh; }

void RenderWidget::initializeGL()
{
  qtr::Logger::log()->trace("RenderWidget::initializeGL");

  this->makeCurrent();

  this->initializeOpenGLFunctions();

  // --- Shaders

  qtr::Logger::log()->trace("RenderWidget::initializeGL: setting up shaders...");

  this->sp_shader_manager->add_shader_from_code("diffuse_basic",
                                                diffuse_basic_vertex,
                                                diffuse_basic_frag);

  this->sp_shader_manager->add_shader_from_code("diffuse_phong",
                                                diffuse_basic_vertex,
                                                diffuse_phong_frag);

  this->sp_shader_manager->add_shader_from_code("diffuse_blinn_phong",
                                                diffuse_basic_vertex,
                                                diffuse_blinn_phong_frag);

  this->sp_shader_manager->add_shader_from_code("depth_map",
                                                depth_map_vertex,
                                                depth_map_frag);

  this->sp_shader_manager->add_shader_from_code("shadow_map_depth_pass",
                                                shadow_map_depth_pass_vertex,
                                                shadow_map_depth_pass_frag);

  this->sp_shader_manager->add_shader_from_code("shadow_map_lit_pass",
                                                shadow_map_lit_pass_vertex,
                                                shadow_map_lit_pass_frag);

  this->sp_shader_manager->add_shader_from_code("viewer2d_cmap",
                                                viewer2d_cmap_vertex,
                                                viewer2d_cmap_frag);

  // --- Meshes

  generate_plane(this->plane,
                 0.f,
                 -1e-3f,
                 0.f,
                 2000.f * this->hmap_w,
                 2000.f * this->hmap_w);

  // --- Textures

  // depth buffer
  {
    int depth_map_res = 512;

    this->sp_texture_manager->add_depth_texture(QTR_TEX_DEPTH,
                                                depth_map_res,
                                                depth_map_res,
                                                false);

    // create framebuffer for depth map
    glGenFramebuffers(1, &this->fbo_depth);
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_depth);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D,
                           this->sp_texture_manager->get(QTR_TEX_DEPTH)->get_id(),
                           0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // shadow map texture and buffer
  {
    this->sp_texture_manager->add_depth_texture(QTR_TEX_SHADOW_MAP,
                                                this->shadow_map_resolution,
                                                this->shadow_map_resolution,
                                                true);

    // create framebuffer for shadow depth
    glGenFramebuffers(1, &this->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D,
                           this->sp_texture_manager->get(QTR_TEX_SHADOW_MAP)->get_id(),
                           0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // --- ImGUI

  qtr::Logger::log()->trace("RenderWidget::initializeGL: setup ImGui context");

  // ImGui context
  IMGUI_CHECKVERSION();
  this->imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext(this->imgui_context);
  ImGui::StyleColorsDark();
  imgui_set_blender_style();

  // OpenGL3 backend
  ImGui_ImplOpenGL3_Init("#version 330");

  // Set initial High-DPI framebuffer scale so ImGui is correct even
  // before the first resizeGL call.
  {
    float dpr = static_cast<float>(this->devicePixelRatioF());
    ImGuiIO &io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2(dpr, dpr);
  }

  this->doneCurrent();

  this->initial_gl_done = true;
}

void RenderWidget::reset_camera_position()
{
  // TODO use json
  this->target = glm::vec3(0.0f, 0.0f, 0.0f);
  this->pan_offset = glm::vec2(0.0f, 0.0f);
  this->distance = 5.0f;
  this->alpha_x = 35.f / 180.f * 3.14f;
  this->alpha_y = -25.f / 180.f * 3.14f;

  this->light_phi = -45.f / 180.f * 3.14f;
  this->light_theta = 30.f / 180.f * 3.14f;

  this->need_update = true;
}

void RenderWidget::reset_heightmap_geometry()
{
  this->makeCurrent();
  this->hmap.destroy();
  if (this->sp_texture_manager->get(QTR_TEX_HMAP))
    this->sp_texture_manager->get(QTR_TEX_HMAP)->destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_leaves()
{
  this->makeCurrent();
  this->leaves_instanced_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_path()
{
  this->makeCurrent();
  this->path_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_points()
{
  this->makeCurrent();
  this->points_instanced_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_texture(const std::string &name)
{
  qtr::Logger::log()->trace("RenderWidget::reset_texture: {}", name);

  this->makeCurrent();
  if (this->sp_texture_manager->get(name))
    this->sp_texture_manager->get(name)->destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_textures()
{
  qtr::Logger::log()->trace("RenderWidget::reset_textures");

  this->makeCurrent();

  // /!\ do not reset the depth maps
  const std::vector<std::string> tex_names = {QTR_TEX_ALBEDO,
                                              QTR_TEX_HMAP,
                                              QTR_TEX_NORMAL};
  for (auto &s : tex_names)
    if (this->sp_texture_manager->get(s))
      this->sp_texture_manager->get(s)->destroy();

  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_water_geometry()
{
  this->makeCurrent();
  this->water_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_rocks()
{
  this->makeCurrent();
  this->rocks_instanced_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::reset_trees()
{
  this->makeCurrent();
  this->trees_instanced_mesh.destroy();
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::resizeEvent(QResizeEvent *event)
{
  this->makeCurrent();
  QOpenGLWidget::resizeEvent(event);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::resizeGL(int w, int h)
{
  if (!isValid())
    return;

  this->makeCurrent();

  // Qt passes logical pixels to resizeGL.  On High-DPI displays the
  // actual framebuffer is larger by devicePixelRatioF().  glViewport
  // always operates in physical (framebuffer) pixels.
  float dpr = static_cast<float>(this->devicePixelRatioF());
  int   fb_w = static_cast<int>(w * dpr);
  int   fb_h = static_cast<int>(h * dpr);

  this->glViewport(0, 0, fb_w, fb_h);

  if (this->imgui_context)
  {
    ImGui::SetCurrentContext(this->imgui_context);
    ImGuiIO &io = this->get_imgui_io();
    // DisplaySize stays in logical pixels so that mouse coordinates
    // (which Qt gives in logical pixels) map 1:1.
    io.DisplaySize = ImVec2(float(w), float(h));
    // FramebufferScale tells the ImGui OpenGL backend to multiply by
    // DPR when setting up its own glViewport / scissor rects.
    io.DisplayFramebufferScale = ImVec2(dpr, dpr);
  }
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_bypass_texture_albedo(bool new_state)
{
  this->bypass_texture_albedo = new_state;
  this->need_update = true;
}

void RenderWidget::set_common_uniforms(QOpenGLShaderProgram &shader,
                                       const glm::mat4      &model,
                                       const glm::mat4      &projection,
                                       const glm::mat4      &view,
                                       const glm::mat4      &light_space)
{
  // Textures
  this->sp_texture_manager->bind_and_set(shader);

  // Time & matrices
  shader.setUniformValue("time", time);
  shader.setUniformValue("model", toQMat(model));
  shader.setUniformValue("view", toQMat(view));
  shader.setUniformValue("projection", toQMat(projection));
  shader.setUniformValue("light_space_matrix", toQMat(light_space));

  // Camera & light
  shader.setUniformValue("camera_pos", toQVec(camera.position));
  shader.setUniformValue("view_pos", toQVec(camera.position));
  shader.setUniformValue("light_pos", toQVec(light.position));

  // Screen & depth
  {
    float dpr = static_cast<float>(this->devicePixelRatioF());
    shader.setUniformValue("screen_size",
                           toQVec(glm::vec2(width() * dpr, height() * dpr)));
  }
  shader.setUniformValue("near_plane", camera.near_plane);
  shader.setUniformValue("far_plane", camera.far_plane);

  // Shadow & AO
  shader.setUniformValue("bypass_shadow_map", bypass_shadow_map);
  shader.setUniformValue("shadow_strength", shadow_strength);
  shader.setUniformValue("add_ambiant_occlusion", add_ambiant_occlusion);
  shader.setUniformValue("ambiant_occlusion_strength", ambiant_occlusion_strength);
  shader.setUniformValue("ambiant_occlusion_radius", ambiant_occlusion_radius);

  // Rendering settings
  shader.setUniformValue("has_instances", false);
  shader.setUniformValue("scale_h", scale_h);
  shader.setUniformValue("hmap_h0", this->hmap_h0);
  shader.setUniformValue("hmap_h", this->hmap_h);
  shader.setUniformValue("normal_visualization", normal_visualization);
  shader.setUniformValue("normal_map_scaling", 0.f); // reset by default
  shader.setUniformValue("gamma_correction", gamma_correction);
  shader.setUniformValue("apply_tonemap", apply_tonemap);

  // Effects
  shader.setUniformValue("add_fog", add_fog);
  shader.setUniformValue("fog_color", toQVec(fog_color));
  shader.setUniformValue("fog_density", fog_density);
  shader.setUniformValue("fog_height", fog_height);
  shader.setUniformValue("add_atmospheric_scattering", add_atmospheric_scattering);
  shader.setUniformValue("scattering_density", scattering_density);
  shader.setUniformValue("rayleigh_color", toQVec(rayleigh_color));
  shader.setUniformValue("mie_color", toQVec(mie_color));
  shader.setUniformValue("fog_strength", fog_strength);
  shader.setUniformValue("fog_scattering_ratio", fog_scattering_ratio);

  // Reset per-object flags
  shader.setUniformValue("use_texture_albedo", false);
  shader.setUniformValue("use_water_colors", false);
  shader.setUniformValue("normal_map_scaling", 0.f);
  shader.setUniformValue("shininess", 32.f);
  shader.setUniformValue("spec_strength", 0.f);
}

void RenderWidget::set_heightmap_geometry(const std::vector<float> &data,
                                          int                       width,
                                          int                       height,
                                          bool                      add_skirt)
{
  qtr::Logger::log()->trace("RenderWidget::set_heightmap_geometry");

  this->makeCurrent();

  generate_heightmap(this->hmap,
                     data,
                     width,
                     height,
                     0.f,
                     this->hmap_h0,
                     0.f,
                     this->hmap_w,
                     this->hmap_h,
                     this->hmap_w,
                     add_skirt,
                     /* add_level */ 0.f,
                     /* exclude_below */ -FLT_MAX,
                     &this->hmap_hmin);

  // regenerate plane
  generate_plane(this->plane,
                 0.f,
                 this->hmap_hmin * this->hmap_h - 1e-3f,
                 0.f,
                 2000.f * this->hmap_w,
                 2000.f * this->hmap_w);

  qtr::Logger::log()->trace("RenderWidget::set_heightmap_geometry: w x h = {} x {}",
                            width,
                            height);

  // also generate the heightmap texture /!\ texture of float, scaled
  // as the input, not scaled as what the OpenGL sees (there is an
  // additional this->hmap_h scaling for OpenGL)
  if (this->sp_texture_manager->get(QTR_TEX_HMAP))
    this->sp_texture_manager->get(QTR_TEX_HMAP)->from_float_vector(data, width);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_leaves(const std::vector<float> &x,
                              const std::vector<float> &y,
                              const std::vector<float> &h,
                              const std::vector<float> &radius)
{
  qtr::Logger::log()->trace("RenderWidget::set_leaves");

  this->makeCurrent();

  if (x.size() != y.size() || x.size() != h.size() || x.size() != radius.size())
    throw std::invalid_argument("RenderWidget::set_leaves: vector sizes does not match");

  std::vector<BaseInstance> instances;

  glm::vec3 color = glm::vec3(0.f, 1.f, 0.);

  for (size_t k = 0; k < x.size(); ++k)
  {
    float xs = 0.5f * this->hmap_w * (2.f * x[k] - 1.f);
    float hs = this->hmap_h0 + this->hmap_h * h[k];
    float ys = 0.5f * this->hmap_w * (2.f * y[k] - 1.f);
    float rs = 2.f * radius[k];
    float rotation = (float)std::rand() / RAND_MAX * glm::two_pi<float>();

    instances.push_back({glm::vec3(xs, hs, ys), rs, rotation, color});
  }

  // unit sphere
  auto  mesh = std::make_shared<Mesh>();
  float r = 1.f;
  generate_grass_leaf_2sided(*mesh, glm::vec3(0.f, 0.f, 0.f), r, 0.1f * r);

  this->leaves_instanced_mesh.create(mesh, instances);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_path(const std::vector<float> &x,
                            const std::vector<float> &y,
                            const std::vector<float> &h)
{
  qtr::Logger::log()->trace("RenderWidget::set_path");

  this->makeCurrent();

  if (x.size() != y.size() || x.size() != h.size())
    throw std::invalid_argument("RenderWidget::set_path: vector sizes does not match");

  std::vector<glm::vec3> points;
  for (size_t k = 0; k < x.size(); ++k)
  {
    // rescale to render size
    float xs = 0.5f * this->hmap_w * (2.f * x[k] - 1.f);
    float hs = this->hmap_h0 + this->hmap_h * h[k];
    float ys = 0.5f * this->hmap_w * (2.f * y[k] - 1.f);

    points.push_back(glm::vec3(xs, hs, ys));
  }

  // TODO scale with point value

  generate_path(path_mesh, points, 0.01f);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_points(const std::vector<float> &x,
                              const std::vector<float> &y,
                              const std::vector<float> &h)
{
  qtr::Logger::log()->trace("RenderWidget::set_points");

  this->makeCurrent();

  if (x.size() != y.size() || x.size() != h.size())
    throw std::invalid_argument("RenderWidget::set_points: vector sizes does not match");

  std::vector<BaseInstance> instances;

  float     scale = 0.01f;
  float     rotation = 0.f;
  glm::vec3 color = glm::vec3(0.f, 1.f, 0.);

  for (size_t k = 0; k < x.size(); ++k)
  {
    float xs = 0.5f * this->hmap_w * (2.f * x[k] - 1.f);
    float hs = this->hmap_h0 + this->hmap_h * h[k];
    float ys = 0.5f * this->hmap_w * (2.f * y[k] - 1.f);

    instances.push_back({glm::vec3(xs, hs, ys), scale, rotation, color});
  }

  // unit sphere
  auto sphere_mesh = std::make_shared<Mesh>();
  generate_sphere(*sphere_mesh, 1.f);

  this->points_instanced_mesh.create(sphere_mesh, instances);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_render_type(const RenderType &new_render_type)
{
  this->render_type = new_render_type;
}

void RenderWidget::set_shadow_map_resolution(int resolution)
{
  if (resolution == this->shadow_map_resolution)
    return;

  // Clamp to safe range (prevents crash from invalid resolution values)
  resolution = std::clamp(resolution, 256, 8192);

  this->shadow_map_resolution = resolution;

  // Guard: ensure OpenGL context is valid before manipulating GPU resources
  if (!this->context() || !this->context()->isValid())
  {
    qtr::Logger::log()->warn("set_shadow_map_resolution: no valid GL context, deferring");
    this->need_update = true;
    return;
  }

  this->makeCurrent();

  // destroy old shadow map texture
  Texture *p_old_tex = this->sp_texture_manager->get(QTR_TEX_SHADOW_MAP);
  if (p_old_tex)
    p_old_tex->destroy();

  // destroy old FBO
  if (this->fbo)
  {
    glDeleteFramebuffers(1, &this->fbo);
    this->fbo = 0;
  }

  // recreate shadow map texture at new resolution
  this->sp_texture_manager->add_depth_texture(QTR_TEX_SHADOW_MAP,
                                              resolution,
                                              resolution,
                                              true);

  // recreate FBO
  glGenFramebuffers(1, &this->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_DEPTH_ATTACHMENT,
                         GL_TEXTURE_2D,
                         this->sp_texture_manager->get(QTR_TEX_SHADOW_MAP)->get_id(),
                         0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  this->doneCurrent();
  this->need_update = true;
}

void RenderWidget::set_render_plane(bool new_state)
{
  this->render_plane = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_points(bool new_state)
{
  this->render_points = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_path(bool new_state)
{
  this->render_path = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_hmap(bool new_state)
{
  this->render_hmap = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_rocks(bool new_state)
{
  this->render_rocks = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_trees(bool new_state)
{
  this->render_trees = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_water(bool new_state)
{
  this->render_water = new_state;
  this->need_update = true;
}

void RenderWidget::set_render_leaves(bool new_state)
{
  this->render_leaves = new_state;
  this->need_update = true;
}

void RenderWidget::set_rocks(const std::vector<float> &x,
                             const std::vector<float> &y,
                             const std::vector<float> &h,
                             const std::vector<float> &radius)
{
  qtr::Logger::log()->trace("RenderWidget::set_rocks");

  this->makeCurrent();

  if (x.size() != y.size() || x.size() != h.size() || x.size() != radius.size())
    throw std::invalid_argument("RenderWidget::set_rocks: vector sizes does not match");

  std::vector<BaseInstance> instances;

  glm::vec3 color = glm::vec3(0.f, 1.f, 0.);

  for (size_t k = 0; k < x.size(); ++k)
  {
    float xs = 0.5f * this->hmap_w * (2.f * x[k] - 1.f);
    float hs = this->hmap_h0 + this->hmap_h * h[k];
    float ys = 0.5f * this->hmap_w * (2.f * y[k] - 1.f);
    float rs = 2.f * radius[k];
    float rotation = (float)std::rand() / RAND_MAX * glm::two_pi<float>();

    instances.push_back({glm::vec3(xs, hs, ys), rs, rotation, color});
  }

  // unit sphere
  auto mesh = std::make_shared<Mesh>();
  generate_rock(*mesh, 1.f, 0.3f, 0);

  this->rocks_instanced_mesh.create(mesh, instances);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_texture(const std::string          &name,
                               const std::vector<uint8_t> &data,
                               int                         width)
{
  qtr::Logger::log()->trace("RenderWidget::set_texture: {}", name);

  this->makeCurrent();

  if (this->sp_texture_manager->get(name))
    this->sp_texture_manager->get(name)->from_image_8bit_rgba(data, width);
  this->need_update = true;
}

void RenderWidget::set_trees(const std::vector<float> &x,
                             const std::vector<float> &y,
                             const std::vector<float> &h,
                             const std::vector<float> &radius)
{
  qtr::Logger::log()->trace("RenderWidget::set_trees");

  this->makeCurrent();

  if (x.size() != y.size() || x.size() != h.size() || x.size() != radius.size())
    throw std::invalid_argument("RenderWidget::set_trees: vector sizes does not match");

  std::vector<BaseInstance> instances;

  glm::vec3 color = glm::vec3(0.f, 1.f, 0.);

  for (size_t k = 0; k < x.size(); ++k)
  {
    float xs = 0.5f * this->hmap_w * (2.f * x[k] - 1.f);
    float hs = this->hmap_h0 + this->hmap_h * h[k];
    float ys = 0.5f * this->hmap_w * (2.f * y[k] - 1.f);
    float rs = 2.f * radius[k];
    float rotation = (float)std::rand() / RAND_MAX * glm::two_pi<float>();

    instances.push_back({glm::vec3(xs, hs, ys), rs, rotation, color});
  }

  // unit sphere
  auto  mesh = std::make_shared<Mesh>();
  float r = 1.f;
  generate_tree(*mesh, r, 0.1f * r, 5.f * r, r, 5);

  this->trees_instanced_mesh.create(mesh, instances);
  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::set_water_geometry(const std::vector<float> &data,
                                      int                       width,
                                      int                       height,
                                      float                     exclude_below)
{
  qtr::Logger::log()->trace("RenderWidget::set_water_geometry");

  this->makeCurrent();

  bool  add_skirt = false;
  float add_level = 0.f;

  generate_heightmap(this->water_mesh,
                     data,
                     width,
                     height,
                     0.f,
                     this->hmap_h0,
                     0.f,
                     this->hmap_w,
                     this->hmap_h,
                     this->hmap_w,
                     add_skirt,
                     add_level,
                     exclude_below);

  this->need_update = true;
  this->doneCurrent();
}

void RenderWidget::setup_gl_state()
{
  float dpr = static_cast<float>(this->devicePixelRatioF());
  glViewport(0,
             0,
             static_cast<int>(this->width() * dpr),
             static_cast<int>(this->height() * dpr));
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glPolygonMode(GL_FRONT_AND_BACK, this->wireframe_mode ? GL_LINE : GL_FILL);
}

QSize RenderWidget::sizeHint() const { return QSize(QTR_CONFIG->widget.size_hint); }

void RenderWidget::unbind_textures() { this->sp_texture_manager->unbind(); }

void RenderWidget::update_camera()
{
  this->camera.set_position_angles(this->distance, this->alpha_x, this->alpha_y);

  glm::vec3 pan(this->pan_offset.x * cos(this->alpha_y),
                this->pan_offset.y,
                -this->pan_offset.x * sin(this->alpha_y));

  this->camera.position += pan;
  this->camera.target = this->target + pan;

  if (this->auto_rotate_camera)
  {
    this->alpha_y += 0.5f * this->dt;
    this->need_update = true;
  }
}

void RenderWidget::update_light()
{
  this->light.set_position_spherical(this->light_distance,
                                     this->light_theta,
                                     this->light_phi);

  // actually works with a fixed sun: compensate for the elevation
  // scaling
  this->light.position.y /= this->scale_h;

  if (this->auto_rotate_light)
  {
    this->light_phi += 0.5f * this->dt;
    this->need_update = true;
  }
}

void RenderWidget::update_time()
{
  this->dt = static_cast<float>(this->timer.restart()) / 1000.0f;
  this->time += this->dt;
}

} // namespace qtr
