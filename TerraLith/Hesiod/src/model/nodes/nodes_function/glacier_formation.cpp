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

void setup_glacier_formation_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "ice_mask", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("snow_line", "Snow Line", 0.65f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("accumulation_rate", "Accumulation Rate", 0.3f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("flow_viscosity", "Flow Viscosity", 0.4f, 0.f, 1.f);
  node.add_attr<IntAttribute>("iterations", "Iterations", 20, 1, 100);
  node.add_attr<FloatAttribute>("carve_depth", "Carve Depth", 0.1f, 0.f, 0.5f);
  node.add_attr<FloatAttribute>("moraine_height", "Moraine Height", 0.05f, 0.f, 0.2f);
  node.add_attr<FloatAttribute>("u_shape_power", "U-Shape Power", 2.f, 1.f, 4.f);
  node.add_attr<SeedAttribute>("seed", "Seed");

  // attribute(s) order
  node.set_attr_ordered_key({"_TEXT_Snow & Ice",
                             "snow_line",
                             "accumulation_rate",
                             "seed",
                             "_TEXT_Flow",
                             "flow_viscosity",
                             "iterations",
                             "_TEXT_Valley Shaping",
                             "carve_depth",
                             "moraine_height",
                             "u_shape_power"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_glacier_formation_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (!p_in)
    return;

  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Heightmap *p_ice = node.get_value_ref<hmap::Heightmap>("ice_mask");

  std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

  float snow_line = node.get_attr<FloatAttribute>("snow_line");
  float accum_rate = node.get_attr<FloatAttribute>("accumulation_rate");
  float viscosity = node.get_attr<FloatAttribute>("flow_viscosity");
  int   iterations = node.get_attr<IntAttribute>("iterations");
  float carve_depth = node.get_attr<FloatAttribute>("carve_depth");
  float moraine_h = node.get_attr<FloatAttribute>("moraine_height");
  float u_power = node.get_attr<FloatAttribute>("u_shape_power");

  *p_out = *p_in;

  // Simulate glacier formation per tile
  hmap::transform(
      {p_out, p_mask, p_ice},
      [snow_line, accum_rate, viscosity, iterations, carve_depth, moraine_h, u_power](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float> /* bbox */)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_mask = p_arrays[1];
        hmap::Array *pa_ice = p_arrays[2];

        float hmin = pa_out->min();
        float hmax = pa_out->max();
        float range = hmax - hmin;
        if (range < 1e-6f)
          return;

        // Ice accumulation above snow line
        hmap::Array ice(shape);
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float h_norm = ((*pa_out)(i, j) - hmin) / range;
            float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
            if (h_norm > snow_line)
              ice(i, j) = (h_norm - snow_line) / (1.f - snow_line) * accum_rate * mask_v;
          }

        // Simple diffusion-based flow simulation
        for (int iter = 0; iter < iterations; iter++)
        {
          hmap::Array new_ice = ice;
          for (int j = 1; j < shape.y - 1; j++)
            for (int i = 1; i < shape.x - 1; i++)
            {
              if (ice(i, j) < 1e-6f)
                continue;

              // Flow to lowest neighbor
              float h_center = (*pa_out)(i, j) + ice(i, j);
              float flow = 0.f;
              int best_i = i, best_j = j;
              float best_h = h_center;

              for (int dj = -1; dj <= 1; dj++)
                for (int di = -1; di <= 1; di++)
                {
                  if (di == 0 && dj == 0)
                    continue;
                  float h_n = (*pa_out)(i + di, j + dj) + ice(i + di, j + dj);
                  if (h_n < best_h)
                  {
                    best_h = h_n;
                    best_i = i + di;
                    best_j = j + dj;
                    flow = (h_center - h_n) * viscosity;
                  }
                }

              flow = std::min(flow, ice(i, j) * 0.5f);
              if (flow > 0.f)
              {
                new_ice(i, j) -= flow;
                new_ice(best_i, best_j) += flow;
              }
            }
          ice = new_ice;
        }

        // Carve U-shaped valleys and build moraines
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float ice_v = ice(i, j);
            if (ice_v > 0.01f)
            {
              // Carve valley (U-shape power)
              float carve = carve_depth * std::pow(ice_v, u_power);
              (*pa_out)(i, j) -= carve;
            }

            // Store ice mask
            if (pa_ice)
              (*pa_ice)(i, j) = std::clamp(ice_v, 0.f, 1.f);
          }

        // Simple moraine deposition at ice edges
        if (moraine_h > 0.f)
        {
          for (int j = 1; j < shape.y - 1; j++)
            for (int i = 1; i < shape.x - 1; i++)
            {
              float ice_v = ice(i, j);
              if (ice_v < 0.01f)
              {
                // Check if adjacent to ice
                bool near_ice = false;
                for (int dj = -1; dj <= 1; dj++)
                  for (int di = -1; di <= 1; di++)
                    if (ice(i + di, j + dj) > 0.05f)
                      near_ice = true;

                if (near_ice)
                  (*pa_out)(i, j) += moraine_h;
              }
            }
        }
      },
      node.get_config_ref()->hmap_transform_mode_cpu);

  p_out->smooth_overlap_buffers();

  post_process_heightmap(node, *p_out, p_in);
}

} // namespace hesiod
