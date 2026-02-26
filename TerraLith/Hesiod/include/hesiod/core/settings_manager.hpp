/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <filesystem>
#include <functional>
#include <string>

#include "nlohmann/json.hpp"

namespace hesiod
{

// Unified JSON-backed settings manager for Hesiod 0.6.
// Persists all user preferences to ~/.config/hesiod/settings.json
// and broadcasts changes via a callback.
class SettingsManager
{
public:
  static SettingsManager &instance();

  // Load settings from disk (creates defaults if missing)
  void load();

  // Save current settings to disk
  void save() const;

  // Get the settings file path
  std::filesystem::path get_settings_path() const;

  // --- Interface settings ---
  struct Interface
  {
    bool        enable_node_body_previews = true;
    int         preview_type = 2;     // 0=Gray, 1=Magma, 2=Terrain(hillshade), 3=Histogram
    int         preview_resolution = 256; // 128, 256, 512
    int         grid_style = 2;       // 0=None, 1=Classic, 2=Blueprint subtle
    bool        show_category_icons = true;
  } interface;

  // --- Performance settings ---
  struct Performance
  {
    bool enable_smart_preview_cache = true;
    int  cache_memory_limit_mb = 512;
    bool enable_incremental_evaluation = true;
    int  default_resolution = 2048;   // 1024, 2048, 4096, 8192
    int  default_tiling = 4;          // 2, 4, 8
  } performance;

  // --- Vulkan settings ---
  struct Vulkan
  {
    bool        enable_vulkan_globally = true;
    bool        fallback_to_cpu_on_error = true;
    std::string device_selection = "Auto";
  } vulkan;

  // --- Logging settings ---
  struct Logging
  {
    int  terminal_logging_level = 2;  // 0=Silent, 1=Warning, 2=Info, 3=Debug, 4=Verbose
    bool log_vulkan_timings = true;
    bool show_stutter_warnings = true;
  } logging;

  // --- Node Editor settings ---
  struct NodeEditor
  {
    int  node_rounding_radius = 8;    // 0-16
    int  port_size = 22;              // hit area
    bool fuzzy_search_aliases = true;
    int  duplicate_offset = 220;      // horizontal shift on Ctrl+D
  } node_editor;

  // --- Viewer settings ---
  struct Viewer
  {
    int default_shadow_resolution = 2048; // 1024, 2048, 4096, 8192
    int msaa_level = 2;                   // 0=Off, 1=2x, 2=4x, 3=8x
  } viewer;

  // Notification callback (set by application layer)
  std::function<void()> settings_changed;

  // Serialize to/from JSON
  nlohmann::json to_json() const;
  void           from_json(const nlohmann::json &j);

private:
  SettingsManager() = default;
  SettingsManager(const SettingsManager &) = delete;
  SettingsManager &operator=(const SettingsManager &) = delete;
};

} // namespace hesiod
