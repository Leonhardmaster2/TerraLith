/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
 License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include "qtr/logger.hpp"
#include "qtr/texture.hpp"

namespace qtr
{

Texture::Texture() : id(0), width(0), height(0) {}

Texture::~Texture() { this->destroy(); }

void Texture::bind(int unit)
{
  if (this->is_active())
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, this->id);
  }
}

void Texture::bind_and_set(QOpenGLShaderProgram &shader,
                           const std::string    &tex_id,
                           int                   unit)
{
  if (this->is_active())
  {
    this->bind(unit);
    shader.setUniformValue(tex_id.c_str(), unit);
  }
}

void Texture::destroy()
{
  if (this->is_active())
  {
    glDeleteTextures(1, &this->id);
    this->id = 0;
  }
}

bool Texture::from_float_vector(const std::vector<float> &data, int new_width)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = data.size() / this->width;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_R32F, // internal format (1 float channel)
               this->width,
               this->height,
               0,
               GL_RED,
               GL_FLOAT,
               data.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool Texture::from_image_8bit_grayscale(const std::vector<uint8_t> &img, int new_width)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = img.size() / this->width;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_R8, // explicitly specify 8-bit internal format (for Windows)
               this->width,
               this->height,
               0,
               GL_RED,
               GL_UNSIGNED_BYTE, // uint8_t
               img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool Texture::from_image_8bit_rgb(const std::vector<uint8_t> &img, int new_width)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = img.size() / 3 / this->width;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  qtr::Logger::log()->trace("Texture::from_image_8bit_rgb: id = {}, w x h = {} x {}",
                            this->id,
                            this->width,
                            this->height);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGB,
               this->width,
               this->height,
               0,
               GL_RGB,
               GL_UNSIGNED_BYTE, // uint8_t
               img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool Texture::from_image_8bit_rgba(const std::vector<uint8_t> &img, int new_width)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = img.size() / 4 / this->width;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  qtr::Logger::log()->trace("Texture::from_image_8bit_rgba: id = {}, w x h = {} x {}",
                            this->id,
                            this->width,
                            this->height);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               this->width,
               this->height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE, // uint8_t
               img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool Texture::from_image_16bit_grayscale(const std::vector<uint16_t> &img, int new_width)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = img.size() / this->width;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_R16, // explicitly specify 16-bit internal format (for Windows)
               this->width,
               this->height,
               0,
               GL_RED,
               GL_UNSIGNED_SHORT, // uint16_t
               img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

void Texture::generate_depth_texture(int  new_width,
                                     int  new_height,
                                     bool force_border_color)
{
  this->initializeOpenGLFunctions();
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();
  this->destroy();

  this->width = new_width;
  this->height = new_height;

  glGenTextures(1, &this->id);
  glBindTexture(GL_TEXTURE_2D, this->id);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_DEPTH_COMPONENT32F,
               this->width,
               this->height,
               0,
               GL_DEPTH_COMPONENT,
               GL_FLOAT,
               nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  if (force_border_color)
  {
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture::get_id() const { return this->id; }

int Texture::get_width() const { return this->width; }

int Texture::get_height() const { return this->height; }

bool Texture::is_active() const { return (this->id != 0); }

void Texture::unbind()
{
  if (this->is_active())
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace qtr
