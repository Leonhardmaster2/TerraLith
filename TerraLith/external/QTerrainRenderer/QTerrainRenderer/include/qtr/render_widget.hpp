/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QTimer>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "nlohmann/json.hpp"

#include "qtr/camera.hpp"
#include "qtr/instanced_mesh.hpp"
#include "qtr/light.hpp"
#include "qtr/mesh.hpp"
#include "qtr/shader_manager.hpp"
#include "qtr/texture.hpp"
#include "qtr/texture_manager.hpp"

#define QTR_TEX_ALBEDO "albedo"
#define QTR_TEX_HMAP "hmap"
#define QTR_TEX_NORMAL "normal"
#define QTR_TEX_SHADOW_MAP "shadow_map"
#define QTR_TEX_DEPTH "depth"

namespace qtr
{

enum RenderType : int
{
  RENDER_2D,
  RENDER_3D
};

struct Viewer2DSettings
{
  float     zoom = 0.9f;
  glm::vec2 offset = glm::vec2(0.f, 0.f);
  bool      hillshading = true;
  float     sun_azimuth = 90.f / 180.f * 3.14f;
  float     sun_zenith = 60.f / 180.f * 3.14f;

  enum Colormap : int
  {
    // dont change order (for OpenGL)
    GRAY,
    VIRIDIS,
    TURBO,
    MAGMA
  } cmap = Colormap::MAGMA;
};

class RenderWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
  Q_OBJECT

public:
  explicit RenderWidget(const std::string &_title = "", QWidget *parent = nullptr);
  ~RenderWidget();

  // --- Serialization
  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  // --- Setters
  void set_render_type(const RenderType &new_render_type);

  bool get_bypass_texture_albedo() const;
  bool get_render_plane() const;
  bool get_render_points() const;
  bool get_render_path() const;
  bool get_render_hmap() const;
  bool get_render_rocks() const;
  bool get_render_trees() const;
  bool get_render_water() const;
  bool get_render_leaves() const;

  void set_bypass_texture_albedo(bool new_state);
  void set_render_plane(bool new_state);
  void set_render_points(bool new_state);
  void set_render_path(bool new_state);
  void set_render_hmap(bool new_state);
  void set_render_rocks(bool new_state);
  void set_render_trees(bool new_state);
  void set_render_water(bool new_state);
  void set_render_leaves(bool new_state);

  int  get_mesh_downsample_level() const { return this->mesh_downsample_level; }
  void set_mesh_downsample_level(int level) { this->mesh_downsample_level = level; }

  int  get_shadow_map_resolution() const { return this->shadow_map_resolution; }
  void set_shadow_map_resolution(int resolution);

  // --- QWidget interface
  QSize sizeHint() const override;

  // --- Geometry
  void clear(); // geom and texture

  Mesh &get_water_mesh();

  void set_heightmap_geometry(const std::vector<float> &data,
                              int                       width,
                              int                       height,
                              bool                      add_skirt = true);
  void reset_heightmap_geometry();

  void set_water_geometry(const std::vector<float> &data,
                          int                       width,
                          int                       height,
                          float                     exclude_below);
  void reset_water_geometry();

  void set_points(const std::vector<float> &x,
                  const std::vector<float> &y,
                  const std::vector<float> &h);
  void reset_points();

  void set_path(const std::vector<float> &x,
                const std::vector<float> &y,
                const std::vector<float> &h);
  void reset_path();

  void set_rocks(const std::vector<float> &x,
                 const std::vector<float> &y,
                 const std::vector<float> &h,
                 const std::vector<float> &radius);
  void reset_rocks();

  void set_trees(const std::vector<float> &x,
                 const std::vector<float> &y,
                 const std::vector<float> &h,
                 const std::vector<float> &radius);
  void reset_trees();

  void set_leaves(const std::vector<float> &x,
                  const std::vector<float> &y,
                  const std::vector<float> &h,
                  const std::vector<float> &radius);
  void reset_leaves();

  // --- Textures
  void set_texture(const std::string          &name,
                   const std::vector<uint8_t> &data,
                   int                         width); // RGBA 8bit
  void reset_texture(const std::string &name);
  void reset_textures();

protected:
  // --- OpenGL lifecycle
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void resizeEvent(QResizeEvent *event) override;

  // --- Rendering
  void paintGL() override;
  void render_scene_render_2d();
  void render_scene_render_3d();
  void render_ui_render_2d();
  void render_ui_render_3d();
  void render_depth_map(const glm::mat4 &model,
                        const glm::mat4 &view,
                        const glm::mat4 &projection);
  void render_shadow_map(const glm::mat4 &model, glm::mat4 &light_space_matrix);
  void set_common_uniforms(QOpenGLShaderProgram &shader,
                           const glm::mat4      &model,
                           const glm::mat4      &projection,
                           const glm::mat4      &view,
                           const glm::mat4      &light_space);
  void setup_gl_state();
  void unbind_textures();
  void update_camera();
  void update_light();
  void update_time();

  // --- Input forwarding to ImGui
  ImGuiIO &get_imgui_io();
  void     mousePressEvent(QMouseEvent *e) override;
  void     mouseReleaseEvent(QMouseEvent *e) override;
  void     mouseMoveEvent(QMouseEvent *e) override;
  void     wheelEvent(QWheelEvent *e) override;
  void     keyPressEvent(QKeyEvent *e) override;
  void     keyReleaseEvent(QKeyEvent *e) override;

private:
  // --- Helpers
  void reset_camera_position();

  // --- General
  std::string title;
  RenderType  render_type = RenderType::RENDER_3D;

  // --- GUI state
  QTimer               frame_timer;
  bool                 need_update = false;
  bool                 rotating = false;
  bool                 panning = false;
  std::array<float, 2> last_mouse_pos;

  // --- Timing
  QElapsedTimer timer;
  float         time = 0.f;
  float         dt = 0.f;

  // --- User parameters
  bool wireframe_mode = false;
  bool auto_rotate_light = false;
  bool auto_rotate_camera = false;

  // --- Camera parameters (see reset_camera_position)
  glm::vec3 target;      // Orbit center
  glm::vec2 pan_offset;  // Panning offset
  float     distance;    // Zoom (distance to target)
  float     alpha_x;     // Rotation around X (pitch)
  float     alpha_y;     // Rotation around Y (yaw)
  float     light_phi;   // azimuth
  float     light_theta; // zenith
  float     light_distance = 10.f;

  // --- Heightmap
  float scale_h = 1.0f;
  float hmap_h0 = 0.f;   // hmap zero level
  float hmap_hmin = 0.f; // hmap min level
  float hmap_hmax = 0.f; // hmap max level
  float hmap_w = 2.f;    // width of sides
  float hmap_h = 0.4f;   // elevations scaling (at input)
  int   current_width = 0;
  int   current_height = 0;
  bool  current_add_skirt_state = true;

  // Mesh resolution: power-of-2 downsample (0=Full, 1=1/2, 2=1/4, 3=1/8)
  int mesh_downsample_level = 0;

  // --- Rendering parameters

  // Scene components visibility
  bool render_plane = true;
  bool render_points = true;
  bool render_path = true;
  bool render_hmap = true;
  bool render_rocks = true;
  bool render_trees = true;
  bool render_water = true;
  bool render_leaves = true;

  // Normals
  bool  normal_visualization = false;
  float normal_map_scaling = 1.f;

  // Gamma & tonemap
  float gamma_correction = 2.f;
  bool  apply_tonemap = false;

  // Shadows
  int   shadow_map_resolution = 2048;
  bool  bypass_shadow_map = false;
  float shadow_strength = 0.9f;

  // Ambient occlusion
  bool  add_ambiant_occlusion = true;
  float ambiant_occlusion_strength = 0.8f;
  float ambiant_occlusion_radius = 0.03f;

  // Textures control
  bool bypass_texture_albedo = false;

  // --- Water
  glm::vec3 color_shallow_water;
  glm::vec3 color_deep_water;
  float     water_color_depth = 0.015f;
  float     water_spec_strength = 0.5f;

  // Foam
  bool      add_water_foam = true;
  glm::vec3 foam_color = glm::vec3(1.f, 1.f, 1.f);
  float     foam_depth = 0.005f;

  // Waves
  bool  add_water_waves = true;
  float angle_spread_ratio = 0.f;
  float waves_alpha = 30.f / 180.f * 3.14f;
  float waves_kw = 256.f;
  float waves_amplitude = 0.005f;
  float waves_normal_amplitude = 0.02f;
  bool  animate_waves = false;
  float waves_speed = 0.2f;

  // --- Environmental effects
  bool      add_fog = false;
  glm::vec3 fog_color = glm::vec3(1.f, 1.f, 1.f);
  float     fog_density = 50.0f;
  float     fog_height = 0.1f;
  bool      add_atmospheric_scattering = false;
  float     scattering_density = 0.1f;
  glm::vec3 rayleigh_color = glm::vec3(0.4f, 0.6f, 1.0f); // bluish
  glm::vec3 mie_color = glm::vec3(1.0f, 0.8f, 0.7f);      // whitish/yellowish
  float     fog_strength = 0.5f;
  float     fog_scattering_ratio = 0.7f;

  // --- 2D Viewer
  Viewer2DSettings viewer2d_settings;

  // --- OpenGL resources
  std::unique_ptr<ShaderManager> sp_shader_manager;
  GLuint                         fbo;
  GLuint                         fbo_depth;
  bool                           initial_gl_done = false;

  // --- Scene components
  Camera camera_shadow_pass;
  Camera camera;
  Light  light;

  Mesh                        plane;
  Mesh                        hmap;
  Mesh                        water_mesh;
  Mesh                        path_mesh;
  InstancedMesh<BaseInstance> points_instanced_mesh;
  InstancedMesh<BaseInstance> trees_instanced_mesh;
  InstancedMesh<BaseInstance> rocks_instanced_mesh;
  InstancedMesh<BaseInstance> leaves_instanced_mesh;

  std::unique_ptr<TextureManager> sp_texture_manager;

  // --- ImGUI
  ImGuiContext *imgui_context = nullptr;
};

// --- Helpers
inline QMatrix4x4 toQMat(const glm::mat4 &m)
{
  return QMatrix4x4(glm::value_ptr(glm::transpose(m)));
}

inline QVector3D toQVec(const glm::vec3 &v) { return QVector3D(v.x, v.y, v.z); }
inline QVector2D toQVec(const glm::vec2 &v) { return QVector2D(v.x, v.y); }

} // namespace qtr
