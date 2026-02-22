/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>

#include <QOpenGLFunctions_3_3_Core>

#include "qtr/shader.hpp"

namespace qtr
{

class Texture : protected QOpenGLFunctions_3_3_Core
{
public:
  Texture();
  ~Texture();

  bool from_float_vector(const std::vector<float> &data, int new_width);
  bool from_image_8bit_grayscale(const std::vector<uint8_t> &img, int new_width);
  bool from_image_8bit_rgb(const std::vector<uint8_t> &img, int new_width);
  bool from_image_8bit_rgba(const std::vector<uint8_t> &img, int new_width);
  bool from_image_16bit_grayscale(const std::vector<uint16_t> &img, int new_width);
  void generate_depth_texture(int new_width, int new_height, bool force_border_color);

  GLuint get_id() const;
  int    get_width() const;
  int    get_height() const;

  void bind(int unit = 0);
  void bind_and_set(QOpenGLShaderProgram &shader, const std::string &tex_id, int unit);
  void unbind();
  void destroy();
  bool is_active() const;

private:
  GLuint id;
  int    width;
  int    height;
};

} // namespace qtr