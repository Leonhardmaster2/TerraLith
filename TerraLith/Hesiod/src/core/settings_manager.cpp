/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <fstream>
#include <iostream>

#include "hesiod/core/settings_manager.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

SettingsManager &SettingsManager::instance()
{
  static SettingsManager inst;
  return inst;
}

std::filesystem::path SettingsManager::get_settings_path() const
{
  // ~/.config/hesiod/settings.json
  std::filesystem::path config_dir;

#ifdef _WIN32
  const char *appdata = std::getenv("APPDATA");
  if (appdata)
    config_dir = std::filesystem::path(appdata) / "hesiod";
  else
    config_dir = std::filesystem::path(".") / ".hesiod";
#else
  const char *xdg = std::getenv("XDG_CONFIG_HOME");
  if (xdg)
    config_dir = std::filesystem::path(xdg) / "hesiod";
  else
  {
    const char *home = std::getenv("HOME");
    if (home)
      config_dir = std::filesystem::path(home) / ".config" / "hesiod";
    else
      config_dir = std::filesystem::path(".") / ".hesiod";
  }
#endif

  return config_dir / "settings.json";
}

void SettingsManager::load()
{
  auto path = get_settings_path();

  if (!std::filesystem::exists(path))
  {
    Logger::log()->info("SettingsManager: no settings file found, using defaults");
    save(); // create with defaults
    return;
  }

  try
  {
    std::ifstream file(path);
    if (file.is_open())
    {
      nlohmann::json j;
      file >> j;
      from_json(j);
      Logger::log()->info("SettingsManager: loaded settings from {}", path.string());
    }
  }
  catch (const std::exception &e)
  {
    Logger::log()->warn("SettingsManager: failed to load settings: {}", e.what());
  }
}

void SettingsManager::save() const
{
  auto path = get_settings_path();

  try
  {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(path);
    if (file.is_open())
    {
      file << to_json().dump(2) << "\n";
      Logger::log()->trace("SettingsManager: saved settings to {}", path.string());
    }
  }
  catch (const std::exception &e)
  {
    Logger::log()->error("SettingsManager: failed to save settings: {}", e.what());
  }
}

nlohmann::json SettingsManager::to_json() const
{
  nlohmann::json j;

  // Interface
  j["interface"]["enable_node_body_previews"] = interface.enable_node_body_previews;
  j["interface"]["preview_type"] = interface.preview_type;
  j["interface"]["preview_resolution"] = interface.preview_resolution;
  j["interface"]["grid_style"] = interface.grid_style;
  j["interface"]["show_category_icons"] = interface.show_category_icons;

  // Performance
  j["performance"]["enable_smart_preview_cache"] = performance.enable_smart_preview_cache;
  j["performance"]["cache_memory_limit_mb"] = performance.cache_memory_limit_mb;
  j["performance"]["enable_incremental_evaluation"] =
      performance.enable_incremental_evaluation;
  j["performance"]["default_resolution"] = performance.default_resolution;
  j["performance"]["default_tiling"] = performance.default_tiling;

  // Vulkan
  j["vulkan"]["enable_vulkan_globally"] = vulkan.enable_vulkan_globally;
  j["vulkan"]["fallback_to_cpu_on_error"] = vulkan.fallback_to_cpu_on_error;
  j["vulkan"]["device_selection"] = vulkan.device_selection;

  // Logging
  j["logging"]["terminal_logging_level"] = logging.terminal_logging_level;
  j["logging"]["log_vulkan_timings"] = logging.log_vulkan_timings;
  j["logging"]["show_stutter_warnings"] = logging.show_stutter_warnings;

  // Node Editor
  j["node_editor"]["node_rounding_radius"] = node_editor.node_rounding_radius;
  j["node_editor"]["port_size"] = node_editor.port_size;
  j["node_editor"]["fuzzy_search_aliases"] = node_editor.fuzzy_search_aliases;
  j["node_editor"]["duplicate_offset"] = node_editor.duplicate_offset;

  // Viewer
  j["viewer"]["default_shadow_resolution"] = viewer.default_shadow_resolution;
  j["viewer"]["msaa_level"] = viewer.msaa_level;

  return j;
}

void SettingsManager::from_json(const nlohmann::json &j)
{
  auto safe_get = [](const nlohmann::json &obj,
                     const std::string    &key,
                     auto                 &target)
  {
    if (obj.contains(key))
    {
      try
      {
        target = obj.at(key).get<std::decay_t<decltype(target)>>();
      }
      catch (...)
      {
      }
    }
  };

  if (j.contains("interface"))
  {
    auto &s = j["interface"];
    safe_get(s, "enable_node_body_previews", interface.enable_node_body_previews);
    safe_get(s, "preview_type", interface.preview_type);
    safe_get(s, "preview_resolution", interface.preview_resolution);
    safe_get(s, "grid_style", interface.grid_style);
    safe_get(s, "show_category_icons", interface.show_category_icons);
  }

  if (j.contains("performance"))
  {
    auto &s = j["performance"];
    safe_get(s, "enable_smart_preview_cache", performance.enable_smart_preview_cache);
    safe_get(s, "cache_memory_limit_mb", performance.cache_memory_limit_mb);
    safe_get(s, "enable_incremental_evaluation", performance.enable_incremental_evaluation);
    safe_get(s, "default_resolution", performance.default_resolution);
    safe_get(s, "default_tiling", performance.default_tiling);
  }

  if (j.contains("vulkan"))
  {
    auto &s = j["vulkan"];
    safe_get(s, "enable_vulkan_globally", vulkan.enable_vulkan_globally);
    safe_get(s, "fallback_to_cpu_on_error", vulkan.fallback_to_cpu_on_error);
    safe_get(s, "device_selection", vulkan.device_selection);
  }

  if (j.contains("logging"))
  {
    auto &s = j["logging"];
    safe_get(s, "terminal_logging_level", logging.terminal_logging_level);
    safe_get(s, "log_vulkan_timings", logging.log_vulkan_timings);
    safe_get(s, "show_stutter_warnings", logging.show_stutter_warnings);
  }

  if (j.contains("node_editor"))
  {
    auto &s = j["node_editor"];
    safe_get(s, "node_rounding_radius", node_editor.node_rounding_radius);
    safe_get(s, "port_size", node_editor.port_size);
    safe_get(s, "fuzzy_search_aliases", node_editor.fuzzy_search_aliases);
    safe_get(s, "duplicate_offset", node_editor.duplicate_offset);
  }

  if (j.contains("viewer"))
  {
    auto &s = j["viewer"];
    safe_get(s, "default_shadow_resolution", viewer.default_shadow_resolution);
    safe_get(s, "msaa_level", viewer.msaa_level);
  }
}

} // namespace hesiod
