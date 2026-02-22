/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General Public
 * License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include "hesiod/model/graph/bake_config.hpp"

namespace hesiod
{

// =====================================
// BakeConfigDialog
// =====================================
class BakeConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit BakeConfigDialog(int                        max_size,
                            const BakeConfig          &initial_value,
                            const std::filesystem::path &project_path,
                            QWidget                   *parent = nullptr);

  BakeConfig get_bake_settings() const;

private:
  void update_path_preview();

  // Output group
  QPushButton *browse_button;
  QLineEdit   *path_edit;
  QLineEdit   *base_name_edit;
  QLabel      *path_preview_label;

  // Resolution & format
  QComboBox *resolution_combo;
  QComboBox *format_combo;

  // Variants
  QSlider  *slider;
  QSpinBox *slider_nvariants;

  // Options
  QCheckBox *checkbox_force_distributed;
  QCheckBox *checkbox_force_auto_export;
  QCheckBox *checkbox_rename_export_files;

  QDialogButtonBox *buttons;

  std::filesystem::path project_path;
};

} // namespace hesiod
