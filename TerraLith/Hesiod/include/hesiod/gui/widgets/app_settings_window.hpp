/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QComboBox>
#include <QFormLayout>
#include <QObject>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

namespace hesiod
{

class AppSettingsWindow : public QWidget
{
  Q_OBJECT
public:
  explicit AppSettingsWindow(QWidget *parent = nullptr);

private:
  void setup_layout();

  // Tab builders
  QWidget *create_interface_tab();
  QWidget *create_performance_tab();
  QWidget *create_vulkan_tab();
  QWidget *create_logging_tab();
  QWidget *create_node_editor_tab();
  QWidget *create_viewer_tab();

  // Helpers
  void add_description(QFormLayout *layout, const std::string &description,
                       int max_length = 64);
  void add_title(QFormLayout *layout, const std::string &label,
                 int font_size_delta = 2);
  void bind_bool(QFormLayout *layout, const std::string &label, bool &state,
                 const std::string &tooltip = "");
  void bind_combo(QFormLayout *layout, const std::string &label, int &value,
                  const std::vector<std::string> &options,
                  const std::string &tooltip = "");
  void bind_spinbox(QFormLayout *layout, const std::string &label, int &value,
                    int min_val, int max_val,
                    const std::string &tooltip = "");

  QTabWidget *tab_widget;
};

} // namespace hesiod
