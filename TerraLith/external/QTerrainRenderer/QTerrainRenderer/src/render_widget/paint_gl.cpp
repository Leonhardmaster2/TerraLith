/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include <stdexcept>

#include <QOpenGLFunctions>

#include <backends/imgui_impl_opengl3.h>
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

void RenderWidget::paintGL()
{
  if (!this->initial_gl_done)
    return;

  if (QOpenGLContext::currentContext() != this->context())
    this->makeCurrent();

  this->update_time();
  this->update_light();
  this->update_camera();

  // scene and UI
  switch (this->render_type)
  {
  case RenderType::RENDER_2D:
    this->render_scene_render_2d();
    this->render_ui_render_2d();
    break;
    //
  case RenderType::RENDER_3D:
    this->render_scene_render_3d();
    this->render_ui_render_3d();
    break;
  }

#ifdef __linux__
  this->doneCurrent();
#endif
}

} // namespace qtr
