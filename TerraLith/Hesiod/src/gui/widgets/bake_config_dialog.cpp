/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QFileDialog>
#include <QVBoxLayout>

#include "hesiod/gui/widgets/bake_config_dialog.hpp"

namespace hesiod
{

BakeConfigDialog::BakeConfigDialog(int                          max_size,
                                   const BakeConfig            &initial_value,
                                   const std::filesystem::path &project_path,
                                   QWidget                     *parent)
    : QDialog(parent), project_path(project_path)
{
  this->setWindowTitle("Bake and Export");
  this->setModal(true);
  this->setMinimumWidth(500);

  auto *main_layout = new QVBoxLayout;

  // ===== Output group =====
  {
    auto *group = new QGroupBox("Output");
    auto *layout = new QFormLayout;

    // Export folder
    auto *folder_layout = new QHBoxLayout;
    this->path_edit = new QLineEdit(this);
    this->path_edit->setPlaceholderText("Auto (next to project file)");
    if (!initial_value.export_path.empty())
      this->path_edit->setText(QString::fromStdString(initial_value.export_path.string()));
    this->path_edit->setReadOnly(true);
    folder_layout->addWidget(this->path_edit);

    this->browse_button = new QPushButton("Browse...", this);
    folder_layout->addWidget(this->browse_button);

    auto *clear_button = new QPushButton("Auto", this);
    clear_button->setToolTip("Reset to automatic path (next to project file)");
    clear_button->setMaximumWidth(50);
    folder_layout->addWidget(clear_button);

    layout->addRow("Export folder:", folder_layout);

    // Base name
    this->base_name_edit = new QLineEdit(this);
    this->base_name_edit->setPlaceholderText("Auto (use project name)");
    if (!initial_value.base_name.empty())
      this->base_name_edit->setText(QString::fromStdString(initial_value.base_name));
    layout->addRow("Base name:", this->base_name_edit);

    // Preview
    this->path_preview_label = new QLabel(this);
    this->path_preview_label->setStyleSheet("color: gray; font-style: italic;");
    this->path_preview_label->setWordWrap(true);
    layout->addRow("Output preview:", this->path_preview_label);

    group->setLayout(layout);
    main_layout->addWidget(group);

    // Connections
    QObject::connect(this->browse_button,
                     &QPushButton::clicked,
                     [this]()
                     {
                       QString dir = QFileDialog::getExistingDirectory(
                           this,
                           "Select Export Folder",
                           this->path_edit->text().isEmpty()
                               ? QString::fromStdString(
                                     this->project_path.parent_path().string())
                               : this->path_edit->text());
                       if (!dir.isEmpty())
                       {
                         this->path_edit->setText(dir);
                         this->update_path_preview();
                       }
                     });

    QObject::connect(clear_button,
                     &QPushButton::clicked,
                     [this]()
                     {
                       this->path_edit->clear();
                       this->update_path_preview();
                     });

    QObject::connect(this->base_name_edit,
                     &QLineEdit::textChanged,
                     [this]() { this->update_path_preview(); });
  }

  // ===== Resolution & Format group =====
  {
    auto *group = new QGroupBox("Resolution and Format");
    auto *layout = new QFormLayout;

    // Resolution
    this->resolution_combo = new QComboBox(this);
    for (int size = 2; size <= max_size; size *= 2)
      this->resolution_combo->addItem(QString("%1 x %1").arg(size), size);
    this->resolution_combo->setCurrentIndex(
        this->resolution_combo->findData(initial_value.resolution));
    layout->addRow("Resolution:", this->resolution_combo);

    // Format
    this->format_combo = new QComboBox(this);
    this->format_combo->addItem("Use node settings", -1);
    this->format_combo->addItem("PNG (8 bit)", 0);
    this->format_combo->addItem("PNG (16 bit)", 1);
    this->format_combo->addItem("RAW (16 bit, Unity)", 2);
    this->format_combo->addItem("R16 (16 bit)", 3);
    this->format_combo->addItem("R32 (32 bit float)", 4);
    int format_index = this->format_combo->findData(initial_value.format_override);
    this->format_combo->setCurrentIndex(format_index >= 0 ? format_index : 0);
    layout->addRow("Format:", this->format_combo);

    group->setLayout(layout);
    main_layout->addWidget(group);
  }

  // ===== Variants group =====
  {
    auto *group = new QGroupBox("Variants");
    auto *layout = new QHBoxLayout;

    this->slider = new QSlider(Qt::Horizontal, this);
    this->slider_nvariants = new QSpinBox(this);

    this->slider->setRange(0, 50);
    this->slider_nvariants->setRange(0, 50);
    this->slider_nvariants->setValue(initial_value.nvariants);
    this->slider->setValue(initial_value.nvariants);

    layout->addWidget(new QLabel("Count:"));
    layout->addWidget(this->slider);
    layout->addWidget(this->slider_nvariants);

    QObject::connect(this->slider,
                     &QSlider::valueChanged,
                     this->slider_nvariants,
                     &QSpinBox::setValue);
    QObject::connect(this->slider_nvariants,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     this->slider,
                     &QSlider::setValue);

    group->setLayout(layout);
    main_layout->addWidget(group);
  }

  // ===== Options group =====
  {
    auto *group = new QGroupBox("Options");
    auto *layout = new QVBoxLayout;

    this->checkbox_force_distributed = new QCheckBox("Force distributed computation", this);
    this->checkbox_force_auto_export = new QCheckBox("Force auto export for export nodes",
                                                     this);
    this->checkbox_rename_export_files = new QCheckBox("Add prefix to export filenames", this);

    this->checkbox_force_distributed->setChecked(initial_value.force_distributed);
    this->checkbox_force_auto_export->setChecked(initial_value.force_auto_export);
    this->checkbox_rename_export_files->setChecked(initial_value.rename_export_files);

    layout->addWidget(this->checkbox_force_distributed);
    layout->addWidget(this->checkbox_force_auto_export);
    layout->addWidget(this->checkbox_rename_export_files);

    group->setLayout(layout);
    main_layout->addWidget(group);
  }

  // ===== Buttons =====
  this->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       this);
  main_layout->addWidget(this->buttons);

  this->setLayout(main_layout);

  QObject::connect(this->buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  QObject::connect(this->buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  // Initial preview
  this->update_path_preview();
}

void BakeConfigDialog::update_path_preview()
{
  std::filesystem::path preview_path;

  // Determine export folder
  if (!this->path_edit->text().isEmpty())
  {
    preview_path = this->path_edit->text().toStdString();
  }
  else
  {
    // Auto path: derive from project
    std::filesystem::path ppath = this->project_path.filename();
    if (ppath.empty())
      preview_path = "export";
    else
    {
      preview_path = std::filesystem::path(ppath.string() + "_export");
      if (!this->project_path.parent_path().empty())
        preview_path = this->project_path.parent_path() / preview_path;
    }
  }

  // Determine base name
  std::string bname = this->base_name_edit->text().toStdString();
  if (bname.empty())
  {
    bname = this->project_path.stem().string();
    if (bname.empty())
      bname = "hmap";
  }

  std::filesystem::path example = preview_path / (bname + ".png");
  this->path_preview_label->setText(QString::fromStdString(example.string()));
}

BakeConfig BakeConfigDialog::get_bake_settings() const
{
  BakeConfig config;
  config.resolution = this->resolution_combo->currentData().toInt();
  config.nvariants = this->slider_nvariants->value();
  config.force_distributed = this->checkbox_force_distributed->isChecked();
  config.force_auto_export = this->checkbox_force_auto_export->isChecked();
  config.rename_export_files = this->checkbox_rename_export_files->isChecked();
  config.format_override = this->format_combo->currentData().toInt();

  if (!this->path_edit->text().isEmpty())
    config.export_path = this->path_edit->text().toStdString();
  else
    config.export_path = "";

  config.base_name = this->base_name_edit->text().toStdString();

  return config;
}

} // namespace hesiod
