/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <QImage>

#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

#include "qtr/logger.hpp"

namespace qtr
{

std::vector<uint16_t> load_png_as_16bit_grayscale(const std::string &path,
                                                  int               &width,
                                                  int               &height)
{
  QImage img(path.c_str());
  if (img.isNull())
  {
    throw std::runtime_error("Failed to load image: " + path);
  }

  // Convert image to 16-bit grayscale (Qt ≥ 5.13)
  if (img.format() != QImage::Format_Grayscale16)
  {
    img = img.convertToFormat(QImage::Format_Grayscale16);
  }

  width = img.width();
  height = img.height();

  std::vector<uint16_t> data(static_cast<size_t>(width) * height);

  for (int y = 0; y < height; ++y)
  {
    const uint16_t *scanline = reinterpret_cast<const uint16_t *>(img.constScanLine(y));
    std::memcpy(&data[y * width],
                scanline,
                static_cast<size_t>(width) * sizeof(uint16_t));
  }

  return data;
}

std::vector<uint16_t> load_png_as_16bit_rgba(const std::string &path,
                                             int               &width,
                                             int               &height)
{
  QImage img(path.c_str());
  if (img.isNull())
  {
    throw std::runtime_error("Failed to load image: " + path);
  }

  // Convert image to 16-bit RGBA format (Qt ≥ 5.13)
  if (img.format() != QImage::Format_RGBA64)
    img = img.convertToFormat(QImage::Format_RGBA64);

  width = img.width();
  height = img.height();
  const int channel_count = 4; // R, G, B, A

  std::vector<uint16_t> data(static_cast<size_t>(width) * height * channel_count);

  for (int y = 0; y < height; ++y)
  {
    const uint16_t *scanline = reinterpret_cast<const uint16_t *>(img.constScanLine(y));
    std::memcpy(&data[y * width * channel_count],
                scanline,
                static_cast<size_t>(width) * channel_count * sizeof(uint16_t));
  }

  return data; // RVO handles return efficiently, no need for std::move
}

std::vector<uint8_t> load_png_as_8bit_rgb(const std::string &path,
                                          int               &width,
                                          int               &height)
{
  QImage img(path.c_str());
  if (img.isNull())
  {
    throw std::runtime_error("Failed to load image: " + path);
  }

  // Ensure format is RGB888
  if (img.format() != QImage::Format_RGB888)
  {
    img = img.convertToFormat(QImage::Format_RGB888);
  }

  width = img.width();
  height = img.height();
  const int    channel_count = 3; // R, G, B
  const size_t total_bytes = static_cast<size_t>(width) * height * channel_count;

  std::vector<uint8_t> data(total_bytes);
  std::memcpy(data.data(), img.constBits(), total_bytes);

  return data; // RVO handles return efficiently, no need for std::move
}

std::vector<uint8_t> load_png_as_8bit_rgba(const std::string &path,
                                           int               &width,
                                           int               &height)
{
  QImage img(path.c_str());
  if (img.isNull())
  {
    throw std::runtime_error("Failed to load image: " + path);
  }

  // Ensure format is RGB888
  if (img.format() != QImage::Format_RGBA8888)
  {
    img = img.convertToFormat(QImage::Format_RGBA8888);
  }

  width = img.width();
  height = img.height();
  const int    channel_count = 4; // R, G, B, A
  const size_t total_bytes = static_cast<size_t>(width) * height * channel_count;

  std::vector<uint8_t> data(total_bytes);
  std::memcpy(data.data(), img.constBits(), total_bytes);

  return data; // RVO handles return efficiently, no need for std::move
}

std::vector<float> load_png_as_grayscale(const std::string &path, int &width, int &height)
{
  QImage img(path.c_str());
  if (img.isNull())
  {
    throw std::runtime_error("Failed to load image: " + path);
  }

  // Convert to 16-bit grayscale
  if (img.format() != QImage::Format_Grayscale16)
    img = img.convertToFormat(QImage::Format_Grayscale16);

  width = img.width();
  height = img.height();

  std::vector<float> data(width * height);
  const uint16_t    *src = reinterpret_cast<const uint16_t *>(img.constBits());

  for (int i = 0; i < width * height; ++i)
    data[i] = static_cast<float>(src[i]) / 65535.0f; // Normalize

  return data;
}

} // namespace qtr
