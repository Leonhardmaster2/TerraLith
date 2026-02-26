/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/heightmap.hpp"
#include "highmap/geometry/cloud.hpp"
#include "highmap/primitives.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"

using namespace attr;

namespace hesiod
{

void setup_tree_placement_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "terrain");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "density_mask");
  node.add_port<hmap::Cloud>(gnode::PortType::OUT, "positions");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "canopy_map", CONFIG(node));

  // attribute(s)
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<IntAttribute>("max_trees", "Max Trees", 500, 10, 10000);
  node.add_attr<FloatAttribute>("min_spacing", "Min Spacing", 0.02f, 0.001f, 0.2f);
  node.add_attr<FloatAttribute>("slope_limit", "Max Slope", 30.f, 0.f, 80.f, "{:.0f}");
  node.add_attr<FloatAttribute>("min_elevation", "Min Elevation", 0.1f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("max_elevation", "Max Elevation", 0.7f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("canopy_radius", "Canopy Radius", 0.015f, 0.001f, 0.1f);
  node.add_attr<FloatAttribute>("height_variation", "Height Variation", 0.3f, 0.f, 1.f);

  // attribute(s) order
  node.set_attr_ordered_key({"seed",
                             "max_trees",
                             "min_spacing",
                             "_TEXT_Placement Constraints",
                             "slope_limit",
                             "min_elevation",
                             "max_elevation",
                             "_TEXT_Canopy",
                             "canopy_radius",
                             "height_variation"});
}

void compute_tree_placement_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_terrain = node.get_value_ref<hmap::Heightmap>("terrain");

  if (!p_terrain)
    return;

  hmap::Heightmap *p_density = node.get_value_ref<hmap::Heightmap>("density_mask");
  hmap::Cloud     *p_positions = node.get_value_ref<hmap::Cloud>("positions");
  hmap::Heightmap *p_canopy = node.get_value_ref<hmap::Heightmap>("canopy_map");

  uint  seed = node.get_attr<SeedAttribute>("seed");
  int   max_trees = node.get_attr<IntAttribute>("max_trees");
  float min_spacing = node.get_attr<FloatAttribute>("min_spacing");
  float slope_limit = node.get_attr<FloatAttribute>("slope_limit");
  float min_elev = node.get_attr<FloatAttribute>("min_elevation");
  float max_elev = node.get_attr<FloatAttribute>("max_elevation");
  float canopy_r = node.get_attr<FloatAttribute>("canopy_radius");
  float height_var = node.get_attr<FloatAttribute>("height_variation");

  // Flatten the terrain for sampling
  hmap::Array terrain_flat = p_terrain->to_array();
  float tmin = terrain_flat.min();
  float tmax = terrain_flat.max();
  if (tmax - tmin > 1e-6f)
    terrain_flat = (terrain_flat - tmin) / (tmax - tmin);

  hmap::Vec2<int> shape = terrain_flat.shape;

  // Simple Poisson disk-like placement using rejection sampling
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist_x(0.f, 1.f);
  std::uniform_real_distribution<float> dist_y(0.f, 1.f);

  hmap::Cloud result;
  std::vector<std::pair<float, float>> placed;

  for (int attempt = 0; attempt < max_trees * 10 && (int)placed.size() < max_trees;
       attempt++)
  {
    float px = dist_x(rng);
    float py = dist_y(rng);

    int ix = std::clamp((int)(px * shape.x), 0, shape.x - 1);
    int iy = std::clamp((int)(py * shape.y), 0, shape.y - 1);

    float elevation = terrain_flat(ix, iy);

    // Elevation check
    if (elevation < min_elev || elevation > max_elev)
      continue;

    // Density mask check
    if (p_density)
    {
      hmap::Array density_flat = p_density->to_array();
      float density = density_flat(ix, iy);
      if (dist_x(rng) > density)
        continue;
    }

    // Slope check (approximate gradient)
    float slope = 0.f;
    if (ix > 0 && ix < shape.x - 1 && iy > 0 && iy < shape.y - 1)
    {
      float dx = (terrain_flat(ix + 1, iy) - terrain_flat(ix - 1, iy)) * shape.x;
      float dy = (terrain_flat(ix, iy + 1) - terrain_flat(ix, iy - 1)) * shape.y;
      slope = std::atan(std::sqrt(dx * dx + dy * dy)) * 180.f / M_PI;
    }
    if (slope > slope_limit)
      continue;

    // Minimum spacing check
    bool too_close = false;
    for (const auto &[ex, ey] : placed)
    {
      float d = std::sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey));
      if (d < min_spacing)
      {
        too_close = true;
        break;
      }
    }
    if (too_close)
      continue;

    placed.push_back({px, py});

    // Add with slight height variation
    float tree_height = 1.f - height_var * dist_x(rng);
    result.add_point(hmap::Point(px, py, tree_height));
  }

  *p_positions = result;

  // Generate canopy density map
  if (p_canopy)
  {
    // Clear and fill with canopy splats
    hmap::transform(*p_canopy,
                    [](hmap::Array &x) { x = 0.f; });

    for (size_t k = 0; k < result.get_npoints(); k++)
    {
      float cx = result.points[k].x;
      float cy = result.points[k].y;
      float cv = result.points[k].v;

      // Splat canopy onto heightmap tiles
      hmap::transform(
          *p_canopy,
          [cx, cy, cv, canopy_r](hmap::Array    &x,
                                 hmap::Vec2<int> /* shape */,
                                 hmap::Vec4<float> bbox)
          {
            for (int j = 0; j < x.shape.y; j++)
              for (int i = 0; i < x.shape.x; i++)
              {
                float px = bbox.a + (bbox.b - bbox.a) * (float)i / (float)x.shape.x;
                float py = bbox.c + (bbox.d - bbox.c) * (float)j / (float)x.shape.y;
                float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
                if (d < canopy_r)
                {
                  float falloff = 1.f - d / canopy_r;
                  x(i, j) = std::max(x(i, j), cv * falloff * falloff);
                }
              }
          });
    }
  }
}

} // namespace hesiod
