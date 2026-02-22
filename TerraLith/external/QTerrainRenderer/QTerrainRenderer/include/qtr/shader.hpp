/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

namespace qtr
{

// small wrapper for convenience and safety
class Shader : protected QOpenGLFunctions_3_3_Core
{
public:
  Shader() = default;
  ~Shader();

  bool from_code(const std::string &vertex_code, const std::string &fragment_code);
  bool from_file(const std::string &vertex_path, const std::string &fragment_path);

  QOpenGLShaderProgram       *get();
  const QOpenGLShaderProgram *get() const;

private:
  void destroy();

  std::unique_ptr<QOpenGLShaderProgram> sp_program;
};

static const std::string diffuse_basic_vertex =
#include "shaders/diffuse_basic.vert"
    ;

static const std::string diffuse_basic_frag =
#include "shaders/diffuse_basic.frag"
    ;

static const std::string diffuse_phong_frag =
#include "shaders/diffuse_phong.frag"
    ;

static const std::string diffuse_blinn_phong_frag =
#include "shaders/diffuse_blinn_phong.frag"
    ;

static const std::string depth_map_vertex =
#include "shaders/depth_map.vert"
    ;

static const std::string depth_map_frag =
#include "shaders/depth_map.frag"
    ;

static const std::string shadow_map_depth_pass_vertex =
#include "shaders/shadow_map_depth_pass.vert"
    ;

static const std::string shadow_map_depth_pass_frag =
#include "shaders/shadow_map_depth_pass.frag"
    ;

static const std::string shadow_map_lit_pass_vertex =
#include "shaders/shadow_map_lit_pass.vert"
    ;

static const std::string shadow_map_lit_pass_frag =
#include "shaders/shadow_map_lit_pass.frag"
    ;

static const std::string viewer2d_cmap_vertex =
#include "shaders/viewer2d_cmap.vert"
    ;

static const std::string viewer2d_cmap_frag =
#include "shaders/viewer2d_cmap.frag"
    ;

} // namespace qtr