/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
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
  this->setWindowTitle("Hesiod - Application Settings");
  this->setup_layout();
}

void AppSettingsWindow::add_description(QFormLayout        *form,
                                        const std::string  &description,
                                        int                 max_length)
{
  if (description.empty())
    return;

  QLabel *label = new QLabel(wrap_text(description, max_length).c_str());

  std::string style = std::format(
      "color: {};",
      HSD_CTX.app_settings.colors.text_secondary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, -1);

  form->addRow(label);
}

void AppSettingsWindow::add_title(QFormLayout       *form,
                                  const std::string &text,
                                  int                font_size_delta)
{
  if (text.empty())
    return;

  QLabel *label = new QLabel(text.c_str());

  std::string style = std::format(
      "font-weight: bold; color: {};",
      HSD_CTX.app_settings.colors.text_primary.name().toStdString());
  label->setStyleSheet(style.c_str());
  resize_font(label, font_size_delta);

  form->addRow(label);
}

void AppSettingsWindow::bind_bool(QFormLayout       *form,
                                  const std::string &label,
                                  bool              &state,
                                  const std::string &tooltip)
{
  auto *check_box = new QCheckBox();
  check_box->setChecked(state);
  if (!tooltip.empty())
    check_box->setToolTip(tooltip.c_str());

  this->connect(check_box,
                &QCheckBox::toggled,
                this,
                [&state](bool value) { state = value; });

  form->addRow(label.c_str(), check_box);
}

void AppSettingsWindow::bind_combo(QFormLayout                    *form,
                                   const std::string              &label,
                                   int                            &value,
                                   const std::vector<std::string> &options,
                                   const std::string              &tooltip)
{
  auto *combo = new QComboBox();
  for (const auto &opt : options)
    combo->addItem(opt.c_str());
  combo->setCurrentIndex(value);
  if (!tooltip.empty())
    combo->setToolTip(tooltip.c_str());

  this->connect(combo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [&value](int idx) { value = idx; });

  form->addRow(label.c_str(), combo);
}

void AppSettingsWindow::bind_spinbox(QFormLayout       *form,
                                     const std::string &label,
                                     int               &value,
                                     int                min_val,
                                     int                max_val,
                                     const std::string &tooltip)
{
  auto *spin = new QSpinBox();
  spin->setMinimum(min_val);
  spin->setMaximum(max_val);
  spin->setValue(value);
  if (!tooltip.empty())
    spin->setToolTip(tooltip.c_str());

  this->connect(spin,
                QOverload<int>::of(&QSpinBox::valueChanged),
                this,
                [&value](int v) { value = v; });

  form->addRow(label.c_str(), spin);
}

QWidget *AppSettingsWindow::create_interface_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "Display");

  bind_bool(form, "Enable node body previews",
            ctx.app_settings.interface.enable_data_preview_in_node_body,
            "Show live thumbnail in node body");

  bind_combo(form, "Preview type",
             ctx.app_settings.interface.preview_type,
             {"Gray", "Magma", "Terrain (hillshade)", "Histogram"},
             "Type of preview shown in node body");

  bind_combo(form, "Preview resolution",
             ctx.app_settings.interface.preview_resolution,
             {"128x128", "256x256", "512x512"},
             "Resolution of node body previews");

  bind_combo(form, "Node editor grid style",
             ctx.app_settings.interface.grid_style,
             {"None", "Classic", "Blueprint subtle"},
             "Background grid style for the node editor canvas");

  bind_bool(form, "Show category icons in headers",
            ctx.app_settings.interface.show_category_icons,
            "Display SVG icons in node headers");

  add_title(form, "General");

  bind_bool(form, "Enable node settings in node body",
            ctx.app_settings.interface.enable_node_settings_in_node_body);
  bind_bool(form, "Enable tool tips",
            ctx.app_settings.interface.enable_tool_tips);
  bind_bool(form, "Enable texture downloader",
            ctx.app_settings.interface.enable_texture_downloader);
  bind_bool(form, "Enable example selector at startup",
            ctx.app_settings.interface.enable_example_selector_at_startup);

  return widget;
}

QWidget *AppSettingsWindow::create_performance_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "Caching");

  bind_bool(form, "Enable smart preview cache",
            ctx.app_settings.performance.enable_smart_preview_cache,
            "Instant node state on click (critical for workflow speed)");

  bind_spinbox(form, "Cache memory limit (MB)",
               ctx.app_settings.performance.cache_memory_limit_mb,
               64, 4096,
               "Maximum memory for the LRU preview cache");

  bind_bool(form, "Enable incremental evaluation",
            ctx.app_settings.performance.enable_incremental_evaluation,
            "Only recompute dirty nodes and their downstream chain");

  add_title(form, "Defaults");

  bind_combo(form, "Default resolution",
             ctx.app_settings.performance.default_resolution,
             {"1024", "2048", "4096", "8192"},
             "Default heightmap resolution for new projects");

  bind_combo(form, "Default tiling",
             ctx.app_settings.performance.default_tiling,
             {"2x2", "4x4", "8x8"},
             "Default tile subdivision for parallel computation");

  return widget;
}

QWidget *AppSettingsWindow::create_vulkan_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "Vulkan Compute");

#ifdef HESIOD_HAS_VULKAN
  bind_bool(form, "Enable Vulkan globally",
            ctx.app_settings.vulkan_settings.enable_vulkan_globally,
            "Master toggle for Vulkan GPU acceleration");

  bind_bool(form, "Fallback to CPU on error",
            ctx.app_settings.vulkan_settings.fallback_to_cpu_on_error,
            "Automatically retry failed Vulkan operations on CPU");

  add_description(form, "Vulkan device selection is automatic. The first "
                        "available discrete GPU will be used.");

  {
    QLabel *status = new QLabel("Vulkan Compute Available");
    status->setStyleSheet("color: #00FFAA; font-weight: bold;");
    form->addRow("Status", status);
  }
#else
  add_description(form, "Vulkan compute is not available in this build. "
                        "Rebuild with -DHESIOD_ENABLE_VULKAN=ON to enable.");
  {
    QLabel *status = new QLabel("Not Available (OpenCL fallback)");
    status->setStyleSheet("color: #FF8800; font-weight: bold;");
    form->addRow("Status", status);
  }
#endif

  return widget;
}

QWidget *AppSettingsWindow::create_logging_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "Terminal Output");

  bind_combo(form, "Terminal logging level",
             ctx.app_settings.logging_settings.terminal_logging_level,
             {"Silent", "Warning", "Info", "Debug", "Verbose"},
             "Controls verbosity of console output");

  bind_bool(form, "Log Vulkan timings",
            ctx.app_settings.logging_settings.log_vulkan_timings,
            "Show per-node milliseconds in console for Vulkan operations");

  bind_bool(form, "Show stutter warnings",
            ctx.app_settings.logging_settings.show_stutter_warnings,
            "Display yellow warning when a node takes >150 ms");

  return widget;
}

QWidget *AppSettingsWindow::create_node_editor_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "Node Appearance");

  bind_spinbox(form, "Node rounding radius (px)",
               ctx.app_settings.node_editor.node_rounding_radius,
               0, 16,
               "Corner radius for node rectangles");

  bind_spinbox(form, "Port size (px)",
               ctx.app_settings.node_editor.port_size,
               12, 32,
               "Port hit area diameter");

  add_title(form, "Behavior");

  bind_bool(form, "Fuzzy search aliases enabled",
            ctx.app_settings.node_editor.fuzzy_search_aliases,
            "Enable aliases like 'mtn', 'tree', 'ridge', 'lava'");

  {
    // Duplicate offset needs a float->int conversion
    int dup_offset = static_cast<int>(
        ctx.app_settings.node_editor.position_delta_when_duplicating_node);
    auto *spin = new QSpinBox();
    spin->setMinimum(50);
    spin->setMaximum(500);
    spin->setValue(dup_offset);
    spin->setToolTip("Horizontal shift when duplicating nodes with Ctrl+D");

    this->connect(
        spin,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        [&ctx](int v)
        { ctx.app_settings.node_editor.position_delta_when_duplicating_node = (float)v; });

    form->addRow("Duplicate offset (px)", spin);
  }

  add_title(form, "Groups");

  bind_bool(form, "Enable node groups",
            ctx.app_settings.node_editor.enable_node_groups);

  return widget;
}

QWidget *AppSettingsWindow::create_viewer_tab()
{
  auto *widget = new QWidget();
  auto *form = new QFormLayout(widget);
  form->setHorizontalSpacing(24);

  AppContext &ctx = HSD_CTX;

  add_title(form, "3D Viewport");

  bind_combo(form, "Default shadow resolution",
             ctx.app_settings.viewer.default_shadow_resolution,
             {"1024", "2048", "4096", "8192"},
             "Shadow map resolution (prevents crash on resolution change)");

  bind_combo(form, "MSAA level",
             ctx.app_settings.viewer.msaa_level,
             {"Off", "2x", "4x (Recommended)", "8x"},
             "Multi-sample anti-aliasing quality");

  bind_bool(form, "Add heightmap skirt",
            ctx.app_settings.viewer.add_heighmap_skirt);

  return widget;
}

void AppSettingsWindow::setup_layout()
{
  Logger::log()->trace("AppSettingsWindow::setup_layout");

  AppContext &ctx = HSD_CTX;

  auto *main_layout = new QVBoxLayout(this);

  // Header
  {
    auto *header_layout = new QHBoxLayout();
    QLabel *icon = new QLabel;
    icon->setPixmap(QIcon(ctx.app_settings.global.icon_path.c_str()).pixmap(48, 48));
    header_layout->addWidget(icon);

    QLabel *title = new QLabel("Hesiod Application Settings");
    std::string style = std::format(
        "font-weight: bold; font-size: 16px; color: {};",
        ctx.app_settings.colors.text_primary.name().toStdString());
    title->setStyleSheet(style.c_str());
    header_layout->addWidget(title);
    header_layout->addStretch();

    main_layout->addLayout(header_layout);
  }

  // Tab widget with 6 tabs
  tab_widget = new QTabWidget();
  tab_widget->addTab(create_interface_tab(), "Interface");
  tab_widget->addTab(create_performance_tab(), "Performance");
  tab_widget->addTab(create_vulkan_tab(), "Vulkan");
  tab_widget->addTab(create_logging_tab(), "Logging");
  tab_widget->addTab(create_node_editor_tab(), "Node Editor");
  tab_widget->addTab(create_viewer_tab(), "Viewer");

  main_layout->addWidget(tab_widget);

  // Version info footer
  {
    auto *footer = new QFormLayout();

    std::string version_str = std::format("v{}.{}.{}",
                                          HESIOD_VERSION_MAJOR,
                                          HESIOD_VERSION_MINOR,
                                          HESIOD_VERSION_PATCH);

    QLabel *version_label = new QLabel(version_str.c_str());
    std::string vstyle = std::format(
        "color: {}; font-weight: bold;",
        ctx.app_settings.colors.text_primary.name().toStdString());
    version_label->setStyleSheet(vstyle.c_str());
    footer->addRow("Version", version_label);

#ifdef HESIOD_HAS_VULKAN
    QLabel *gpu_label = new QLabel("Vulkan Compute");
    gpu_label->setStyleSheet("color: #00FFAA; font-weight: bold;");
    footer->addRow("GPU Backend", gpu_label);
#else
    QLabel *gpu_label = new QLabel("OpenCL");
    footer->addRow("GPU Backend", gpu_label);
#endif

    main_layout->addLayout(footer);
  }
}

} // namespace hesiod
