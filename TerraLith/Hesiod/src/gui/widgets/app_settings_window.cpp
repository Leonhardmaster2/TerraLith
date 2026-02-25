/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <format>

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/gui/widgets/app_settings_window.hpp"
#include "hesiod/gui/widgets/gui_utils.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/utils.hpp"

namespace hesiod
{

AppSettingsWindow::AppSettingsWindow(QWidget *parent) : QWidget(parent)
{
  Logger::log()->trace("AppSettingsWindow::AppSettingsWindow");

  this->setWindowTitle("Hesiod - Application settings");

  this->setup_layout();
}

void AppSettingsWindow::add_description(const std::string &description, int max_length)
{
  if (description.empty())
    return;

  QLabel *label = new QLabel(wrap_text(description, max_length).c_str());

  std::string style = std::format(
      "color: {};",
      HSD_CTX.app_settings.colors.text_secondary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, -1);

  layout->addRow(label);
}

void AppSettingsWindow::add_title(const std::string &text, int font_size_delta)
{
  if (text.empty())
    return;

  QLabel *label = new QLabel(text.c_str());

  std::string style = std::format(
      "font-weight: bold; color: {};",
      HSD_CTX.app_settings.colors.text_primary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, font_size_delta);

  layout->addRow(label);
}

void AppSettingsWindow::bind_bool(const std::string &label, bool &state)
{
  auto *check_box = new QCheckBox();
  check_box->setChecked(state);

  this->connect(check_box,
                &QCheckBox::toggled,
                this,
                [&state](bool value) { state = value; });

  layout->addRow(label.c_str(), check_box);
}

void AppSettingsWindow::setup_layout()
{
  Logger::log()->trace("AppSettingsWindow::setup_layout");

  AppContext &ctx = HSD_CTX;

  this->layout = new QFormLayout(this);
  this->layout->setHorizontalSpacing(24);

  {
    QLabel *label = new QLabel;
    label->setPixmap(QIcon(ctx.app_settings.global.icon_path.c_str()).pixmap(64, 64));
    this->layout->addRow(label);
  }

  this->add_title("Hesiod - Application Settings", 4);

  this->add_description("Adjust the behavior and appearance of the application. Some "
                        "changes require a restart to take effect.");
  this->add_description("\n");

  // --- Global

  this->add_title("Global");

  this->bind_bool("Create a backup file whenever saving",
                  ctx.app_settings.global.save_backup_file);
  this->add_description("\n");

  // --- Interface

  this->add_title("Interface");

  this->bind_bool("Enable data preview in node body",
                  ctx.app_settings.interface.enable_data_preview_in_node_body);
  this->bind_bool("Enable node settings in node body",
                  ctx.app_settings.interface.enable_node_settings_in_node_body);
  this->bind_bool("Enable tool tips", ctx.app_settings.interface.enable_tool_tips);
  this->bind_bool("Enable texture downloader",
                  ctx.app_settings.interface.enable_texture_downloader);
  this->bind_bool("Enable example selector at startup",
                  ctx.app_settings.interface.enable_example_selector_at_startup);

  this->add_title("Viewport");

  this->bind_bool("Add border skirt to the heightmap",
                  ctx.app_settings.viewer.add_heighmap_skirt);

  this->add_description("\n");

  // --- Version info

  this->add_title("About");

  {
    std::string version_str = std::format("v{}.{}.{}",
                                          HESIOD_VERSION_MAJOR,
                                          HESIOD_VERSION_MINOR,
                                          HESIOD_VERSION_PATCH);

    QLabel *version_label = new QLabel(version_str.c_str());

    std::string style = std::format(
        "color: {}; font-weight: bold;",
        HSD_CTX.app_settings.colors.text_primary.name().toStdString());
    version_label->setStyleSheet(style.c_str());

    layout->addRow("Version", version_label);
  }

#ifdef HESIOD_HAS_VULKAN
  {
    QLabel *gpu_label = new QLabel("Vulkan Compute");

    std::string style = std::format(
        "color: #00FFAA; font-weight: bold;",
        HSD_CTX.app_settings.colors.text_primary.name().toStdString());
    gpu_label->setStyleSheet(style.c_str());

    layout->addRow("GPU Backend", gpu_label);
  }
#else
  {
    QLabel *gpu_label = new QLabel("OpenCL");
    layout->addRow("GPU Backend", gpu_label);
  }
#endif
}

} // namespace hesiod
