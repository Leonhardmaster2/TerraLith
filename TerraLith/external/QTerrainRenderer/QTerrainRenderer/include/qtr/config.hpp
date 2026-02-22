/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <memory>

#include <QSize>

#define QTR_CONFIG qtr::Config::get_config()

namespace qtr
{

class Config
{
public:
  Config() = default;
  static std::shared_ptr<Config> &get_config();

  struct Widget
  {
    QSize size_hint = QSize(1024, 768);
  } widget;

  struct Viewer3D
  {
    bool show_mouse_control = false;
  } viewer3d;

private:
  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;

  // static member to hold the singleton instance
  static std::shared_ptr<Config> instance;
};

} // namespace qtr
