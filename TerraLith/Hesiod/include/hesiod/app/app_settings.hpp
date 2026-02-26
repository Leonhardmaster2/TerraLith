/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QColor>
#include <QIcon>

#include "nlohmann/json.hpp"

#define HSD_ICON(name)                                                                   \
  static_cast<hesiod::HesiodApplication *>(QCoreApplication::instance())                 \
      ->get_context()                                                                    \
      .app_settings.icons.get(name)

namespace hesiod
{

struct AppSettings
{
  AppSettings() = default;

  // --- Serialization
  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  // --- Data
  struct Model
  {
    bool allow_broadcast_receive_within_same_graph = true;
  } model;

  struct Colors
  {
    QColor bg_deep{"#191919"};
    QColor bg_primary{"#2B2B2B"};
    QColor bg_secondary{"#4B4B4B"};
    QColor text_primary{"#F4F4F5"};
    QColor text_secondary{"#949495"};
    QColor text_disabled{"#3C3C3C"};
    QColor accent{"#5E81AC"};
    QColor accent_bw{"#FFFFFF"};
    QColor border{"#5B5B5B"};
    QColor hover{"#8B8B8B"};
    QColor pressed{"#ABABAB"};
    QColor separator{"#ABABAB"};
  } colors;

  struct Icons
  {
    explicit Icons();
    QIcon get(const std::string &name) const;

    std::map<std::string, QIcon> icons_map; // populated in ctor
  } icons;

  struct Global
  {
    std::string icon_path = "data/hesiod_icon.png";
    std::string default_startup_project_file = "data/default.hsd";
    std::string quick_start_html_file = "data/quick_start.html";
    std::string node_documentation_path = "data/node_documentation.json";
    std::string git_version_file = "data/git_version.txt";
    std::string ready_made_path = "data/bootstraps";
    bool        save_backup_file = true;
    std::string online_help_url = "https://hesioddoc.readthedocs.io/en/latest/";
  } global;

  struct Interface
  {
    bool enable_data_preview_in_node_body = true;
    bool enable_node_settings_in_node_body = false;
    bool enable_texture_downloader = true;
    bool enable_tool_tips = true;
    bool enable_example_selector_at_startup = true;

    // 0.6 additions
    int  preview_type = 2;       // 0=Gray, 1=Magma, 2=Terrain(hillshade), 3=Histogram
    int  preview_resolution = 256; // 128, 256, 512
    int  grid_style = 2;         // 0=None, 1=Classic, 2=Blueprint subtle
    bool show_category_icons = true;
  } interface;

  // 0.6: Performance tab settings
  struct Performance
  {
    bool enable_smart_preview_cache = true;
    int  cache_memory_limit_mb = 512;
    bool enable_incremental_evaluation = true;
    int  default_resolution = 2048;  // 1024, 2048, 4096, 8192
    int  default_tiling = 4;         // 2x2, 4x4, 8x8
  } performance;

  // 0.6: Vulkan tab settings
  struct VulkanSettings
  {
    bool        enable_vulkan_globally = true;
    bool        fallback_to_cpu_on_error = true;
    std::string device_selection = "Auto";
  } vulkan_settings;

  // 0.6: Logging tab settings
  struct LoggingSettings
  {
    int  terminal_logging_level = 2; // 0=Silent, 1=Warning, 2=Info, 3=Debug, 4=Verbose
    bool log_vulkan_timings = true;
    bool show_stutter_warnings = true;
  } logging_settings;

  struct NodeEditor
  {
    std::string gpu_device_name = ""; // let CLWrapper decides
    int         default_resolution = 1024;
    int         default_tiling = 4;
    float       default_overlap = 0.5f;
    int         preview_w = 128;
    int         preview_h = 128;
    std::string doc_path = "data/node_documentation.json";
    float       position_delta_when_duplicating_node = 220.f;
    float       auto_layout_dx = 256.f;
    float       auto_layout_dy = 384.f;
    bool        show_node_settings_pan = true;
    bool        show_viewer = true;
    int         max_bake_resolution = 8192 * 4;
    bool        disable_during_update = false;
    bool        enable_node_groups = true;

    // 0.6 additions
    int  node_rounding_radius = 8; // 0-16 px
    int  port_size = 22;           // hit area
    bool fuzzy_search_aliases = true;
  } node_editor;

  struct Viewer
  {
    int  width = 512;
    int  height = 512;
    bool add_heighmap_skirt = true;

    // 0.6 additions
    int  default_shadow_resolution = 2048; // 1024, 2048, 4096, 8192 (prevents crash)
    int  msaa_level = 2;                   // 0=Off, 1=2x, 2=4x, 3=8x
  } viewer;

  struct Window // main window
  {
    int x = 0;
    int y = 0;
    int w = 1024;
    int h = 1024;
    int progress_bar_width = 200;

    int gm_x = 0; // graph manager geometry
    int gm_y = 0;
    int gm_w = 1024;
    int gm_h = 1024;

    bool show_graph_manager_widget = false;
    bool show_texture_downloader_widget = false;
  } window;
};

} // namespace hesiod