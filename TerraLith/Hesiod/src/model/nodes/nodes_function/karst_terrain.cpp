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

void setup_karst_terrain_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<FloatAttribute>("dissolution_rate", "Dissolution Rate", 0.3f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("sinkhole_density", "Sinkhole Density", 0.2f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("sinkhole_depth", "Sinkhole Depth", 0.15f, 0.f, 0.5f);
  node.add_attr<FloatAttribute>("sinkhole_radius", "Sinkhole Radius", 0.03f, 0.005f, 0.1f);
  node.add_attr<FloatAttribute>("tower_density", "Tower/Pinnacle Density", 0.1f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("tower_height", "Tower Height", 0.2f, 0.f, 0.5f);
  node.add_attr<FloatAttribute>("surface_roughness", "Surface Roughness", 0.3f, 0.f, 1.f);
  node.add_attr<IntAttribute>("iterations", "Iterations", 10, 1, 50);

  // attribute(s) order
  node.set_attr_ordered_key({"seed",
                             "_TEXT_Dissolution",
                             "dissolution_rate",
                             "iterations",
                             "_TEXT_Sinkholes",
                             "sinkhole_density",
                             "sinkhole_depth",
                             "sinkhole_radius",
                             "_TEXT_Towers & Pinnacles",
                             "tower_density",
                             "tower_height",
                             "_TEXT_Surface",
                             "surface_roughness"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_karst_terrain_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (!p_in)
    return;

  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

  uint  seed = node.get_attr<SeedAttribute>("seed");
  float dissolution = node.get_attr<FloatAttribute>("dissolution_rate");
  float sink_density = node.get_attr<FloatAttribute>("sinkhole_density");
  float sink_depth = node.get_attr<FloatAttribute>("sinkhole_depth");
  float sink_radius = node.get_attr<FloatAttribute>("sinkhole_radius");
  float tower_density = node.get_attr<FloatAttribute>("tower_density");
  float tower_height = node.get_attr<FloatAttribute>("tower_height");
  float roughness = node.get_attr<FloatAttribute>("surface_roughness");
  int   iterations = node.get_attr<IntAttribute>("iterations");

  *p_out = *p_in;

  hmap::transform(
      {p_out, p_mask},
      [seed, dissolution, sink_density, sink_depth, sink_radius, tower_density,
       tower_height, roughness, iterations](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_mask = p_arrays[1];

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(0.f, 1.f);

        // Dissolution: preferentially erode low curvature areas
        for (int iter = 0; iter < iterations; iter++)
        {
          for (int j = 1; j < shape.y - 1; j++)
            for (int i = 1; i < shape.x - 1; i++)
            {
              float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;

              // Laplacian curvature
              float lap = ((*pa_out)(i + 1, j) + (*pa_out)(i - 1, j) +
                           (*pa_out)(i, j + 1) + (*pa_out)(i, j - 1) -
                           4.f * (*pa_out)(i, j));

              // Dissolve concave areas more (negative laplacian = concave)
              if (lap < 0.f)
                (*pa_out)(i, j) += lap * dissolution * 0.1f * mask_v;
            }
        }

        // Add sinkholes
        int n_sinkholes = static_cast<int>(sink_density * 50);
        for (int k = 0; k < n_sinkholes; k++)
        {
          float cx = dist(rng);
          float cy = dist(rng);
          float depth = sink_depth * (0.5f + 0.5f * dist(rng));
          float radius = sink_radius * (0.5f + 0.5f * dist(rng));

          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
            {
              float px = bbox.a + (bbox.b - bbox.a) * (float)i / (float)shape.x;
              float py = bbox.c + (bbox.d - bbox.c) * (float)j / (float)shape.y;
              float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
              if (d < radius)
              {
                float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
                float falloff = 1.f - (d / radius);
                (*pa_out)(i, j) -= depth * falloff * falloff * mask_v;
              }
            }
        }

        // Add tower karst pinnacles
        int n_towers = static_cast<int>(tower_density * 30);
        for (int k = 0; k < n_towers; k++)
        {
          float cx = dist(rng);
          float cy = dist(rng);
          float height = tower_height * (0.5f + 0.5f * dist(rng));
          float radius = 0.01f + 0.02f * dist(rng);

          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
            {
              float px = bbox.a + (bbox.b - bbox.a) * (float)i / (float)shape.x;
              float py = bbox.c + (bbox.d - bbox.c) * (float)j / (float)shape.y;
              float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
              if (d < radius)
              {
                float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
                float falloff = 1.f - (d / radius);
                (*pa_out)(i, j) += height * falloff * falloff * falloff * mask_v;
              }
            }
        }

        // Add surface roughness (micro-dissolution texture)
        if (roughness > 0.01f)
        {
          hmap::Array noise = hmap::noise(hmap::NoiseType::PERLIN,
                                          shape,
                                          {32.f, 32.f},
                                          seed + 500,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          bbox);
          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
            {
              float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
              (*pa_out)(i, j) += noise(i, j) * roughness * 0.02f * mask_v;
            }
        }
      },
      node.get_config_ref()->hmap_transform_mode_cpu);

  p_out->smooth_overlap_buffers();

  post_process_heightmap(node, *p_out, p_in);
}

} // namespace hesiod
