/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General Public
 * License. The full license is in the file LICENSE, distributed with this software. */
#pragma once

#include <filesystem>
#include <string>

#include "nlohmann/json.hpp"

namespace hesiod
{

// =====================================
// BakeConfig
// =====================================
struct BakeConfig
{
  int  resolution = 1024;
  int  nvariants = 0;
  bool force_distributed = true;
  bool force_auto_export = true;
  bool rename_export_files = true;

  // Export location and naming
  std::filesystem::path export_path = "";   // empty = auto-derive from project
  std::string           base_name = "";     // empty = use project name
  int                   format_override = -1; // -1 = use node settings, 0=PNG8, 1=PNG16, 2=RAW16

  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;
};

// =====================================
// Functions
// =====================================
void override_export_nodes_settings(const std::string           &fname,
                                    const std::filesystem::path &export_path,
                                    unsigned int                 random_seeds_increment,
                                    const BakeConfig            &bake_settings);

} // namespace hesiod
