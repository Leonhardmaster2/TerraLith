/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <filesystem>
#include <random>

#include "hesiod/gui/widgets/bake_config_dialog.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/utils.hpp"

namespace hesiod
{

// Format override value to choice string mapping
static std::string format_value_to_choice(int format_value)
{
  switch (format_value)
  {
  case 0: return "png (8 bit)";
  case 1: return "png (16 bit)";
  case 2: return "raw (16 bit, Unity)";
  case 3: return "r16 (16 bit)";
  case 4: return "r32 (32 bit float)";
  default: return "";
  }
}

void override_export_nodes_settings(const std::string           &fname,
                                    const std::filesystem::path &export_path,
                                    unsigned int                 random_seeds_increment,
                                    const BakeConfig            &bake_settings)
{
  Logger::log()->trace("override_export_nodes_settings: fname = {}, export_path = {}",
                       fname,
                       export_path.string());

  std::random_device rd;

  // load
  nlohmann::json json = json_from_file(fname);

  // modify
  for (auto &[key, value] : json["graph_manager"]["graph_nodes"].items())
    for (auto &j : value["nodes"])
    {
      std::string node_label = j["label"];

      if (node_label.find("Export") != std::string::npos)
      {
        // force node auto export
        if (bake_settings.force_auto_export)
        {
          if (j.contains("auto_export"))
            j["auto_export"]["value"] = true;
        }

        // override format if requested
        if (bake_settings.format_override >= 0)
        {
          if (j.contains("format"))
          {
            j["format"]["value"] = bake_settings.format_override;
            j["format"]["choice"] = format_value_to_choice(bake_settings.format_override);
          }
        }

        // set export resolution to match the bake resolution
        if (j.contains("export_resolution"))
          j["export_resolution"]["value"] = bake_settings.resolution;

        // Override export path and name (new split attributes)
        if (j.contains("export_path") && j["export_path"].contains("value"))
        {
          j["export_path"]["value"] = export_path.string();
        }

        if (j.contains("export_name") && j["export_name"].contains("value"))
        {
          std::string orig_name = j["export_name"]["value"].get<std::string>();
          std::string label = j.value("label", "");
          std::string id = j.value("id", "");

          std::string new_name;
          if (!bake_settings.base_name.empty())
            new_name = bake_settings.base_name;
          else
            new_name = orig_name;

          if (bake_settings.rename_export_files)
            new_name = label + "_" + id + "_" + new_name;

          j["export_name"]["value"] = new_name;
        }

        // Legacy support: override old 'fname' attribute if present
        if (j.contains("fname") && j["fname"].contains("value"))
        {
          std::filesystem::path basename = std::filesystem::path(
                                               j["fname"]["value"].get<std::string>())
                                               .filename();

          std::string label = j.value("label", "");
          std::string id = j.value("id", "");

          std::string file_basename;
          if (!bake_settings.base_name.empty())
          {
            std::string ext = basename.extension().string();
            if (bake_settings.format_override == 2)
              ext = ".raw";
            else if (bake_settings.format_override == 3)
              ext = ".r16";
            else if (bake_settings.format_override == 4)
              ext = ".r32";
            else if (bake_settings.format_override >= 0)
              ext = ".png";

            file_basename = bake_settings.base_name + ext;
          }
          else
          {
            file_basename = basename.string();
          }

          std::string new_name = bake_settings.rename_export_files
                                     ? label + "_" + id + "_" + file_basename
                                     : file_basename;

          j["fname"]["value"] = (export_path / new_name).string();
        }
      }

      if (random_seeds_increment > 0)
      {
        if (j.contains("seed") && j["seed"].contains("value"))
        {
          unsigned int v = j["seed"]["value"];
          j["seed"]["value"] = v + random_seeds_increment;
        }
      }
    }

  // write back
  json_to_file(json, fname);
}

} // namespace hesiod
