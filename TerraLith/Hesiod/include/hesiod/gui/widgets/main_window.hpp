/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QComboBox>
#include <QDockWidget>
#include <QMainWindow>
#include <QProgressBar>

namespace hesiod
{

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

  void notify(const std::string &msg = "", int timeout = 5000);

  void restore_geometry();
  void save_geometry() const;
  void setup_connections_with_project();

  // --- Dock widget management ---
  void set_viewer_dock_widget(QWidget *viewer);
  void set_settings_dock_widget(QWidget *settings);
  void clear_dock_widgets();

  QDockWidget *get_viewer_dock() const { return this->viewer_dock; }
  QDockWidget *get_settings_dock() const { return this->settings_dock; }

  // --- Resolution ---
  void set_resolution_combo_value(int resolution);

signals:
  void resolution_changed(int new_resolution);

private:
  void setup_progress_bar();
  void setup_resolution_combo();
  void setup_dock_widgets();

  QProgressBar *progress_bar;
  QComboBox    *resolution_combo = nullptr;
  QDockWidget  *viewer_dock = nullptr;
  QDockWidget  *settings_dock = nullptr;
};

} // namespace hesiod
