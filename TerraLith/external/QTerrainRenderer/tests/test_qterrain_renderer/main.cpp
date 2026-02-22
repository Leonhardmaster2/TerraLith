/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <random>

#include <QApplication>

#include "qterrain_renderer.hpp"

int main(int argc, char *argv[])
{
  qtr::Logger::log()->info("testing qterrain_renderer...");

  qputenv("QT_LOGGING_RULES", QTR_QPUTENV_QT_LOGGING_RULES);
  QApplication app(argc, argv);

  qtr::RenderWidget *renderer = new qtr::RenderWidget("Test widget");

  renderer->show();

  // set the heightmap data
  {
    int                width, height;
    std::vector<float> data = qtr::load_png_as_grayscale("hmap.png", width, height);

    renderer->set_heightmap_geometry(data, width, height);
  }

  {
    int                  width, height;
    std::vector<uint8_t> data = qtr::load_png_as_8bit_rgba("texture.png", width, height);

    renderer->set_texture(QTR_TEX_ALBEDO, data, width);
    // renderer->reset_texture(QTR_TEX_ALBEDO);
  }

  {
    int                  width, height;
    std::vector<uint8_t> data = qtr::load_png_as_8bit_rgba("nmap2.png", width, height);

    renderer->set_texture(QTR_TEX_NORMAL, data, width);
    renderer->reset_texture(QTR_TEX_NORMAL);
  }

  {
    renderer->makeCurrent();
    qtr::generate_plane(renderer->get_water_mesh(), 0.f, 0.1f * 0.4f, 0.f, 2.f, 2.f);
    renderer->doneCurrent();
  }

  {
    std::vector<float> x, y, h;
    x = {0.05f, 0.1f, 0.2f, 0.7f, 0.8f};
    y = {0.2f, 0.2f, 0.4f, 0.7f, 0.8f};
    h = {0.8f, 0.2f, 1.f, 0.5f, 0.7f};

    renderer->set_points(x, y, h);
    renderer->reset_points();
  }

  {
    std::vector<float> x, y, h;
    x = {0.05f, 0.1f, 0.2f, 0.7f, 0.8f};
    y = {0.2f, 0.2f, 0.4f, 0.7f, 0.8f};
    h = {0.8f, 0.2f, 1.f, 0.5f, 0.7f};

    renderer->set_path(x, y, h);
    renderer->reset_path();
  }

  {
    size_t             n = 50000;
    std::vector<float> x, y, h, r;

    for (size_t k = 0; k < n; ++k)
    {
      x.push_back((float)std::rand() / RAND_MAX);
      y.push_back((float)std::rand() / RAND_MAX);
      h.push_back(0.1f + 0.f * (float)std::rand() / RAND_MAX);
      r.push_back(0.001f * (float)std::rand() / RAND_MAX);
    }

    // renderer->set_trees(x, y, h, r);
    // renderer->set_leaves(x, y, h, r);
    // renderer->reset_leaves();
  }

  return app.exec();
}
