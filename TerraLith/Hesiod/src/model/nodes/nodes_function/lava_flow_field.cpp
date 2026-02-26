/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/heightmap.hpp"
#include "highmap/opencl/gpu_opencl.hpp"
#include "highmap/primitives.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/post_process.hpp"

using namespace attr;

namespace hesiod
{

void setup_lava_flow_field_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "flow_map", CONFIG(node));

  // attribute(s)
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<FloatAttribute>("source_elevation", "Source Elevation", 0.8f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("viscosity", "Viscosity", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("temperature", "Temperature", 0.8f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("flow_volume", "Flow Volume", 0.5f, 0.f, 1.f);
  node.add_attr<IntAttribute>("iterations", "Iterations", 30, 1, 200);
  node.add_attr<FloatAttribute>("cooling_rate", "Cooling Rate", 0.02f, 0.001f, 0.1f);
  node.add_attr<FloatAttribute>("buildup_height", "Buildup Height", 0.1f, 0.f, 0.3f);
  node.add_attr<FloatAttribute>("surface_texture", "Surface Texture", 0.3f, 0.f, 1.f);
  node.add_attr<IntAttribute>("n_sources", "Number of Sources", 3, 1, 20);

  // attribute(s) order
  node.set_attr_ordered_key({"seed",
                             "n_sources",
                             "source_elevation",
                             "_TEXT_Flow Properties",
                             "viscosity",
                             "temperature",
                             "flow_volume",
                             "iterations",
                             "_TEXT_Solidification",
                             "cooling_rate",
                             "buildup_height",
                             "surface_texture"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_lava_flow_field_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (!p_in)
    return;

  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Heightmap *p_flow = node.get_value_ref<hmap::Heightmap>("flow_map");

  std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

  *p_out = *p_in;

  uint  seed = node.get_attr<SeedAttribute>("seed");
  float source_elev = node.get_attr<FloatAttribute>("source_elevation");
  float viscosity = node.get_attr<FloatAttribute>("viscosity");
  float temperature = node.get_attr<FloatAttribute>("temperature");
  float flow_volume = node.get_attr<FloatAttribute>("flow_volume");
  int   iterations = node.get_attr<IntAttribute>("iterations");
  float cooling = node.get_attr<FloatAttribute>("cooling_rate");
  float buildup = node.get_attr<FloatAttribute>("buildup_height");
  float texture = node.get_attr<FloatAttribute>("surface_texture");
  int   n_sources = node.get_attr<IntAttribute>("n_sources");

  hmap::transform(
      {p_out, p_mask, p_flow},
      [seed, source_elev, viscosity, temperature, flow_volume, iterations,
       cooling, buildup, texture, n_sources](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_mask = p_arrays[1];
        hmap::Array *pa_flow = p_arrays[2];

        float hmin = pa_out->min();
        float hmax = pa_out->max();
        float range = hmax - hmin;
        if (range < 1e-6f)
          return;

        // Initialize flow and temperature maps
        hmap::Array lava(shape);
        hmap::Array temp_map(shape);

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(0.f, 1.f);

        // Place lava sources at high elevation points
        for (int s = 0; s < n_sources; s++)
        {
          float sx = dist(rng);
          float sy = dist(rng);
          int ix = std::clamp((int)(sx * shape.x), 1, shape.x - 2);
          int iy = std::clamp((int)(sy * shape.y), 1, shape.y - 2);

          float h_norm = ((*pa_out)(ix, iy) - hmin) / range;
          if (h_norm >= source_elev * 0.5f)
          {
            lava(ix, iy) = flow_volume;
            temp_map(ix, iy) = temperature;
          }
        }

        // Simulate lava flow
        for (int iter = 0; iter < iterations; iter++)
        {
          hmap::Array new_lava = lava;

          for (int j = 1; j < shape.y - 1; j++)
            for (int i = 1; i < shape.x - 1; i++)
            {
              if (lava(i, j) < 1e-6f || temp_map(i, j) < 0.01f)
                continue;

              float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
              float h_center = (*pa_out)(i, j) + lava(i, j);

              // Flow speed depends on temperature and viscosity
              float flow_rate = temp_map(i, j) * (1.f - viscosity) * 0.2f * mask_v;

              // Find steepest descent
              float max_diff = 0.f;
              int best_di = 0, best_dj = 0;

              for (int dj = -1; dj <= 1; dj++)
                for (int di = -1; di <= 1; di++)
                {
                  if (di == 0 && dj == 0)
                    continue;
                  float h_n = (*pa_out)(i + di, j + dj) + lava(i + di, j + dj);
                  float diff = h_center - h_n;
                  if (diff > max_diff)
                  {
                    max_diff = diff;
                    best_di = di;
                    best_dj = dj;
                  }
                }

              if (max_diff > 0.f && best_di + best_dj != 0)
              {
                float transfer = std::min(flow_rate * max_diff, lava(i, j) * 0.4f);
                new_lava(i, j) -= transfer;
                new_lava(i + best_di, j + best_dj) += transfer;

                // Transfer heat
                temp_map(i + best_di, j + best_dj) = std::max(
                    temp_map(i + best_di, j + best_dj),
                    temp_map(i, j) * 0.9f);
              }

              // Cool down
              temp_map(i, j) -= cooling;
              if (temp_map(i, j) < 0.f)
                temp_map(i, j) = 0.f;
            }

          lava = new_lava;
        }

        // Apply lava to terrain (solidified flow builds up)
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            if (lava(i, j) > 0.01f)
              (*pa_out)(i, j) += lava(i, j) * buildup;

            if (pa_flow)
              (*pa_flow)(i, j) = std::clamp(lava(i, j), 0.f, 1.f);
          }

        // Add surface texture (rough lava surface)
        if (texture > 0.01f)
        {
          hmap::Array noise = hmap::noise(hmap::NoiseType::PERLIN,
                                          shape,
                                          {24.f, 24.f},
                                          seed + 777,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          bbox);
          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
              if (lava(i, j) > 0.01f)
                (*pa_out)(i, j) += noise(i, j) * texture * 0.01f * lava(i, j);
        }
      },
      node.get_config_ref()->hmap_transform_mode_cpu);

  p_out->smooth_overlap_buffers();

  post_process_heightmap(node, *p_out, p_in);
}

} // namespace hesiod
