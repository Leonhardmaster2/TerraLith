/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "qtr/texture.hpp"

namespace qtr
{

class TextureManager
{
public:
  TextureManager() = default;
  ~TextureManager();

  // Add textures from various data sources
  void add(const std::string &name);

  bool add_from_float_vector(const std::string        &name,
                             const std::vector<float> &data,
                             int                       width);

  bool add_from_image_8bit_grayscale(const std::string          &name,
                                     const std::vector<uint8_t> &img,
                                     int                         width);

  bool add_from_image_8bit_rgb(const std::string          &name,
                               const std::vector<uint8_t> &img,
                               int                         width);

  bool add_from_image_8bit_rgba(const std::string          &name,
                                const std::vector<uint8_t> &img,
                                int                         width);

  bool add_from_image_16bit_grayscale(const std::string           &name,
                                      const std::vector<uint16_t> &img,
                                      int                          width);

  void add_depth_texture(const std::string &name,
                         int                width,
                         int                height,
                         bool               force_border_color);

  // Access / management
  void     bind_and_set(QOpenGLShaderProgram &shader);
  Texture *get(const std::string &name);
  void     clear();
  void     resets();
  void     unbind();

private:
  std::map<std::string, std::unique_ptr<Texture>> textures;
};

} // namespace qtr