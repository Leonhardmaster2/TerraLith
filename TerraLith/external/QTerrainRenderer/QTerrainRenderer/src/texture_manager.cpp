/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
 License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include "qtr/logger.hpp"
#include "qtr/texture_manager.hpp"

namespace qtr
{

TextureManager::~TextureManager() { clear(); }

void TextureManager::add(const std::string &name)
{
  auto tex = std::make_unique<Texture>();
  this->textures[name] = std::move(tex);
}

bool TextureManager::add_from_float_vector(const std::string        &name,
                                           const std::vector<float> &data,
                                           int                       width)
{
  auto tex = std::make_unique<Texture>();
  if (!tex->from_float_vector(data, width))
  {
    qtr::Logger::log()->error(
        "TextureManager::add_from_float_vector: texure generation failed");
    return false;
  }

  this->textures[name] = std::move(tex);
  return true;
}

bool TextureManager::add_from_image_8bit_grayscale(const std::string          &name,
                                                   const std::vector<uint8_t> &img,
                                                   int                         width)
{
  auto tex = std::make_unique<Texture>();
  if (!tex->from_image_8bit_grayscale(img, width))
  {
    qtr::Logger::log()->error(
        "TextureManager::add_from_float_vector: texure generation failed");
    return false;
  }

  this->textures[name] = std::move(tex);
  return true;
}

bool TextureManager::add_from_image_8bit_rgb(const std::string          &name,
                                             const std::vector<uint8_t> &img,
                                             int                         width)
{
  auto tex = std::make_unique<Texture>();
  if (!tex->from_image_8bit_rgb(img, width))
  {
    qtr::Logger::log()->error(
        "TextureManager::add_from_float_vector: texure generation failed");
    return false;
  }

  this->textures[name] = std::move(tex);
  return true;
}

bool TextureManager::add_from_image_8bit_rgba(const std::string          &name,
                                              const std::vector<uint8_t> &img,
                                              int                         width)
{
  auto tex = std::make_unique<Texture>();
  if (!tex->from_image_8bit_rgba(img, width))
    return false;

  this->textures[name] = std::move(tex);
  return true;
}

bool TextureManager::add_from_image_16bit_grayscale(const std::string           &name,
                                                    const std::vector<uint16_t> &img,
                                                    int                          width)
{
  auto tex = std::make_unique<Texture>();
  if (!tex->from_image_16bit_grayscale(img, width))
  {
    qtr::Logger::log()->error(
        "TextureManager::add_from_float_vector: texure generation failed");
    return false;
  }

  this->textures[name] = std::move(tex);
  return true;
}

void TextureManager::add_depth_texture(const std::string &name,
                                       int                width,
                                       int                height,
                                       bool               force_border_color)
{
  auto tex = std::make_unique<Texture>();
  tex->generate_depth_texture(width, height, force_border_color);
  this->textures[name] = std::move(tex);
}

void TextureManager::bind_and_set(QOpenGLShaderProgram &shader)
{
  int unit = 0;

  for (auto &[name, sp_tex] : this->textures)
  {
    const std::string tex_id = "texture_" + name;
    sp_tex->bind_and_set(shader, tex_id, unit);
    unit++;
  }
}

void TextureManager::clear()
{
  this->resets();
  this->textures.clear();
}

Texture *TextureManager::get(const std::string &name)
{
  auto     it = this->textures.find(name);
  Texture *ptr = (it != this->textures.end()) ? it->second.get() : nullptr;

  if (!ptr)
    qtr::Logger::log()->error("TextureManager::get: unknown texture id {}", name);

  return ptr;
}

void TextureManager::resets()
{
  for (auto &[_, tex] : this->textures)
  {
    if (tex)
      tex->destroy();
  }
}

void TextureManager::unbind()
{
  for (auto &[_, sp_tex] : this->textures)
    sp_tex->unbind();
}

} // namespace qtr
