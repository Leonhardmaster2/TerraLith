/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include <cstdint>
#include <fstream>

#include "highmap/export.hpp"
#include "highmap/heightmap.hpp"

#include "attributes.hpp"

#include "hesiod/app/enum_mappings.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/node_factory.hpp"
#include "hesiod/model/utils.hpp"

using namespace attr;

namespace hesiod
{

void setup_export_heightmap_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");

  // attribute(s)
  node.add_attr<FilenameAttribute>("export_path",
                                   "Export directory",
                                   std::filesystem::path("."),
                                   std::string(""),
                                   true,
                                   true);
  node.add_attr<StringAttribute>("export_name", "File name", "hmap");
  node.add_attr<EnumAttribute>("format",
                               "format",
                               enum_mappings.heightmap_export_format_map,
                               "png (16 bit)");
  node.add_attr<EnumAttribute>("export_resolution",
                               "Export resolution",
                               enum_mappings.export_resolution_map,
                               "Graph resolution");
  node.add_attr<BoolAttribute>("auto_export", "auto_export", false);

  // attribute(s) order
  node.set_attr_ordered_key(
      {"export_path", "export_name", "format", "export_resolution", "auto_export"});
}

void compute_export_heightmap_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in && node.get_attr<BoolAttribute>("auto_export"))
  {
    // Build full path from separate directory + name + format extension
    std::filesystem::path dir = node.get_attr<FilenameAttribute>("export_path");
    std::string           name = node.get_attr<StringAttribute>("export_name");
    int                   format = node.get_attr<EnumAttribute>("format");

    // Determine extension from format
    std::string ext;
    switch (format)
    {
    case ExportFormat::RAW16BIT: ext = ".raw"; break;
    case ExportFormat::R16BIT: ext = ".r16"; break;
    case ExportFormat::R32BIT: ext = ".r32"; break;
    default: ext = ".png"; break;
    }

    std::filesystem::path fname = dir / (name + ext);

    // Get export resolution (0 = use graph resolution)
    int res = node.get_attr<EnumAttribute>("export_resolution");

    // Convert heightmap to array, optionally at custom resolution
    hmap::Array array;
    if (res > 0)
      array = p_in->to_array(hmap::Vec2<int>(res, res));
    else
      array = p_in->to_array();

    switch (format)
    {
    case ExportFormat::PNG8BIT:
    {
      array.to_png_grayscale(fname.string(), CV_8U);
    }
    break;

    case ExportFormat::PNG16BIT:
    {
      array.to_png_grayscale(fname.string(), CV_16U);
    }
    break;

    case ExportFormat::RAW16BIT:
    case ExportFormat::R16BIT:
    {
      // 16-bit unsigned int, row-major, bottom-to-top, little-endian
      array.to_raw_16bit(fname.string());
    }
    break;

    case ExportFormat::R32BIT:
    {
      // 32-bit float, row-major, bottom-to-top, normalized [0,1]
      const float vmin = array.min();
      const float vmax = array.max();
      float       a = 0.f;
      float       b = 0.f;
      if (vmin != vmax)
      {
        a = 1.f / (vmax - vmin);
        b = -vmin / (vmax - vmin);
      }

      std::ofstream f(fname.string(), std::ios::binary);
      for (int j = array.shape.y - 1; j >= 0; j--)
        for (int i = 0; i < array.shape.x; i++)
        {
          float v = a * array(i, j) + b;
          f.write(reinterpret_cast<const char *>(&v), sizeof(float));
        }
      f.close();
    }
    break;
    }
  }
}

} // namespace hesiod
