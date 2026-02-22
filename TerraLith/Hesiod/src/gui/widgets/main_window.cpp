/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QApplication>
#include <QLabel>
#include <QStatusBar>

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/gui/widgets/main_window.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/graph/graph_manager.hpp"
#include "hesiod/model/graph/graph_node.hpp"

namespace hesiod
{

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  Logger::log()->trace("MainWindow::MainWindow");

  this->restore_geometry();
  this->setup_resolution_combo();
  this->setup_progress_bar();
  this->setup_dock_widgets();
}

void MainWindow::notify(const std::string &msg, int timeout)
{
  this->statusBar()->showMessage(msg.c_str(), timeout);
}

void MainWindow::restore_geometry()
{
  Logger::log()->trace("MainWindow::restore_geometry");

  AppContext &ctx = HSD_CTX;

  this->setGeometry(ctx.app_settings.window.x,
                    ctx.app_settings.window.y,
                    ctx.app_settings.window.w,
                    ctx.app_settings.window.h);
}

void MainWindow::save_geometry() const
{
  Logger::log()->trace("MainWindow::save_geometry");

  AppContext &ctx = HSD_CTX;

  QRect geom = this->geometry();
  ctx.app_settings.window.x = geom.x();
  ctx.app_settings.window.y = geom.y();
  ctx.app_settings.window.w = geom.width();
  ctx.app_settings.window.h = geom.height();
}

void MainWindow::set_resolution_combo_value(int resolution)
{
  if (!this->resolution_combo)
    return;

  // Block signals to prevent triggering a resolution change
  this->resolution_combo->blockSignals(true);

  QString target = QString("%1x%2").arg(resolution).arg(resolution);
  int     idx = this->resolution_combo->findText(target);

  if (idx >= 0)
    this->resolution_combo->setCurrentIndex(idx);

  this->resolution_combo->blockSignals(false);
}

void MainWindow::setup_connections_with_project()
{
  Logger::log()->trace("MainWindow::setup_connections_with_project");

  AppContext &ctx = HSD_CTX;

  // make sure project is ready
  if (!ctx.project_model->get_graph_manager_ref())
  {
    Logger::log()->error("MainWindow::setup_connections_with_project: graph_manager "
                         "model ref is dangling ptr");
    return;
  }

  // GraphNode model -> MainWindow
  ctx.project_model->get_graph_manager_ref()->update_progress = [this](float progress)
  {
    if (progress == 0.f || progress == 100.f)
    {
      this->progress_bar->setValue(0);
      this->progress_bar->setTextVisible(false);

      const std::string message = (progress == 0.f) ? "Updating graph..."
                                                    : "Graph updated successfully.";

      this->notify(message);
      return;
    }

    this->progress_bar->setTextVisible(true);
    this->progress_bar->setValue(static_cast<int>(progress));
  };

  // Resolution combo -> GraphManager
  this->connect(this,
                &MainWindow::resolution_changed,
                this,
                [&ctx](int new_resolution)
                {
                  GraphManager *p_gm = ctx.project_model->get_graph_manager_ref();
                  if (p_gm)
                    p_gm->change_resolution(new_resolution);
                });

  // Sync combo to current project resolution
  {
    GraphManager *p_gm = ctx.project_model->get_graph_manager_ref();
    if (p_gm && !p_gm->get_graph_order().empty())
    {
      const std::string &first_id = p_gm->get_graph_order().front();
      GraphNode         *p_graph = p_gm->get_graph_ref_by_id(first_id);
      if (p_graph)
        this->set_resolution_combo_value(p_graph->get_config_ref()->shape.x);
    }
  }
}

void MainWindow::setup_progress_bar()
{
  Logger::log()->trace("MainWindow::setup_progress_bar");

  AppContext &ctx = HSD_CTX;

  this->progress_bar = new QProgressBar(this);
  this->progress_bar->setRange(0, 100);
  this->progress_bar->setValue(0);
  this->progress_bar->setTextVisible(false);
  this->progress_bar->setFixedWidth(ctx.app_settings.window.progress_bar_width);

  const std::string sheet = std::format(
      R"(
        QProgressBar {{
            border: 0px;
            border-radius: 0px;
            background-color: {};
            height: 8px;
            padding: 0px;
            font-size: 10px;
        }}
        QProgressBar::chunk {{
            background-color: {};
            border-radius: 0px;
            margin: 0px;
        }}
    )",
      ctx.app_settings.colors.bg_deep.name().toStdString(),
      ctx.app_settings.colors.bg_secondary.name().toStdString());

  this->progress_bar->setStyleSheet(sheet.c_str());

  this->statusBar()->addPermanentWidget(this->progress_bar, 0);
}

void MainWindow::setup_resolution_combo()
{
  Logger::log()->trace("MainWindow::setup_resolution_combo");

  AppContext &ctx = HSD_CTX;

  // Resolution label
  QLabel *label = new QLabel("Resolution:", this);

  // Resolution combo box
  this->resolution_combo = new QComboBox(this);
  this->resolution_combo->setToolTip("Global terrain resolution (all graphs)");

  // Power-of-two resolutions: 256 to 8192
  for (int exp = 8; exp <= 13; ++exp)
  {
    int res = 1 << exp;
    this->resolution_combo->addItem(QString("%1x%2").arg(res).arg(res), res);
  }

  // Set default from app settings
  int default_res = ctx.app_settings.node_editor.default_resolution;
  this->set_resolution_combo_value(default_res);

  // Emit signal when user changes the combo
  this->connect(
      this->resolution_combo,
      QOverload<int>::of(&QComboBox::currentIndexChanged),
      this,
      [this](int index)
      {
        if (index < 0)
          return;
        int resolution = this->resolution_combo->itemData(index).toInt();
        Logger::log()->info("MainWindow: resolution changed to {}", resolution);
        Q_EMIT this->resolution_changed(resolution);
      });

  // Add to status bar (left side, before the progress bar)
  this->statusBar()->addWidget(label);
  this->statusBar()->addWidget(this->resolution_combo);
}

void MainWindow::setup_dock_widgets()
{
  Logger::log()->trace("MainWindow::setup_dock_widgets");

  AppContext &ctx = HSD_CTX;

  // --- 3D Viewer dock (top area by default) ---
  this->viewer_dock = new QDockWidget("3D Viewer", this);
  this->viewer_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  this->viewer_dock->setFeatures(QDockWidget::DockWidgetMovable |
                                 QDockWidget::DockWidgetClosable);
  this->viewer_dock->setMinimumHeight(200);
  this->addDockWidget(Qt::TopDockWidgetArea, this->viewer_dock);
  this->viewer_dock->setFloating(false);
  this->viewer_dock->setVisible(ctx.app_settings.node_editor.show_viewer);

  // --- Node Settings dock (right side by default) ---
  this->settings_dock = new QDockWidget("Node Settings", this);
  this->settings_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  this->settings_dock->setFeatures(QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetClosable);
  this->settings_dock->setMinimumWidth(300);
  this->addDockWidget(Qt::RightDockWidgetArea, this->settings_dock);
  this->settings_dock->setFloating(false);
  this->settings_dock->setVisible(ctx.app_settings.node_editor.show_node_settings_pan);
}

void MainWindow::set_viewer_dock_widget(QWidget *viewer)
{
  if (!this->viewer_dock)
    return;

  // release old widget without destroying it
  QWidget *old = this->viewer_dock->widget();
  if (old)
  {
    old->setParent(nullptr);
    old->hide();
  }

  if (viewer)
  {
    this->viewer_dock->setWidget(viewer);
    viewer->show();
  }
}

void MainWindow::set_settings_dock_widget(QWidget *settings)
{
  if (!this->settings_dock)
    return;

  // release old widget without destroying it
  QWidget *old = this->settings_dock->widget();
  if (old)
  {
    old->setParent(nullptr);
    old->hide();
  }

  if (settings)
    this->settings_dock->setWidget(settings);
}

void MainWindow::clear_dock_widgets()
{
  Logger::log()->trace("MainWindow::clear_dock_widgets");

  if (this->viewer_dock)
  {
    QWidget *old = this->viewer_dock->widget();
    if (old)
    {
      old->setParent(nullptr);
      old->hide();
    }
    this->viewer_dock->setWidget(nullptr);
  }

  if (this->settings_dock)
  {
    QWidget *old = this->settings_dock->widget();
    if (old)
    {
      old->setParent(nullptr);
      old->hide();
    }
    this->settings_dock->setWidget(nullptr);
  }
}

} // namespace hesiod
