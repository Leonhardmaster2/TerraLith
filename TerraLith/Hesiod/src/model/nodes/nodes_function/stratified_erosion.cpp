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

void setup_stratified_erosion_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<IntAttribute>("n_layers", "Number of Layers", 6, 2, 20);
  node.add_attr<FloatAttribute>("layer_hardness_variation", "Hardness Variation", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("erosion_strength", "Erosion Strength", 0.3f, 0.f, 1.f);
  node.add_attr<IntAttribute>("iterations", "Iterations", 15, 1, 100);
  node.add_attr<FloatAttribute>("cliff_sharpness", "Cliff Sharpness", 2.f, 0.5f, 8.f);
  node.add_attr<FloatAttribute>("talus_slope", "Talus Slope", 0.3f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("noise_amp", "Noise Amplitude", 0.1f, 0.f, 0.5f);

  // attribute(s) order
  node.set_attr_ordered_key({"seed",
                             "_TEXT_Layer Structure",
                             "n_layers",
                             "layer_hardness_variation",
                             "_TEXT_Erosion",
                             "erosion_strength",
                             "iterations",
                             "_TEXT_Cliff Formation",
                             "cliff_sharpness",
                             "talus_slope",
                             "noise_amp"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_stratified_erosion_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (!p_in)
    return;

  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

  uint  seed = node.get_attr<SeedAttribute>("seed");
  int   n_layers = node.get_attr<IntAttribute>("n_layers");
  float hardness_var = node.get_attr<FloatAttribute>("layer_hardness_variation");
  float erosion_str = node.get_attr<FloatAttribute>("erosion_strength");
  int   iterations = node.get_attr<IntAttribute>("iterations");
  float cliff_sharp = node.get_attr<FloatAttribute>("cliff_sharpness");
  float talus = node.get_attr<FloatAttribute>("talus_slope");
  float noise_amp = node.get_attr<FloatAttribute>("noise_amp");

  *p_out = *p_in;

  hmap::transform(
      {p_out, p_mask},
      [seed, n_layers, hardness_var, erosion_str, iterations, cliff_sharp, talus, noise_amp](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_mask = p_arrays[1];

        float hmin = pa_out->min();
        float hmax = pa_out->max();
        float range = hmax - hmin;
        if (range < 1e-6f)
          return;

        // Generate layer hardness values
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(0.f, 1.f);
        std::vector<float> layer_hardness(n_layers);
        for (int k = 0; k < n_layers; k++)
          layer_hardness[k] = 0.5f + hardness_var * (dist(rng) - 0.5f);

        float layer_thickness = 1.f / (float)n_layers;

        // Stratified erosion: erode soft layers faster
        for (int iter = 0; iter < iterations; iter++)
        {
          for (int j = 1; j < shape.y - 1; j++)
            for (int i = 1; i < shape.x - 1; i++)
            {
              float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
              float h = ((*pa_out)(i, j) - hmin) / range;

              // Determine which layer this point is in
              int layer_idx = std::clamp((int)(h / layer_thickness), 0, n_layers - 1);
              float hardness = layer_hardness[layer_idx];

              // Erosion rate inversely proportional to hardness
              float erosion_rate = erosion_str * (1.f - hardness) * mask_v;

              // Gradient-based erosion (steeper = more erosion)
              float dx = ((*pa_out)(i + 1, j) - (*pa_out)(i - 1, j)) * 0.5f;
              float dy = ((*pa_out)(i, j + 1) - (*pa_out)(i, j - 1)) * 0.5f;
              float slope_mag = std::sqrt(dx * dx + dy * dy);

              // Cliff formation: hard layers resist erosion, creating steps
              float cliff_factor = std::pow(slope_mag, cliff_sharp);
              cliff_factor = std::min(cliff_factor, 1.f);

              (*pa_out)(i, j) -= erosion_rate * cliff_factor * 0.01f * range;

              // Talus deposition at steep slopes
              if (slope_mag > talus && hardness < 0.5f)
              {
                // Deposit at the lowest neighbor
                float h_center = (*pa_out)(i, j);
                for (int dj = -1; dj <= 1; dj++)
                  for (int di = -1; di <= 1; di++)
                  {
                    if (di == 0 && dj == 0)
                      continue;
                    if ((*pa_out)(i + di, j + dj) < h_center)
                    {
                      float deposit = erosion_rate * 0.001f * range;
                      (*pa_out)(i + di, j + dj) += deposit * 0.125f;
                    }
                  }
              }
            }
        }

        // Add micro-noise texture for rock detail
        if (noise_amp > 0.01f)
        {
          hmap::Array detail = hmap::noise(hmap::NoiseType::PERLIN,
                                           shape,
                                           {16.f, 16.f},
                                           seed + 300,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           bbox);
          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
            {
              float mask_v = pa_mask ? (*pa_mask)(i, j) : 1.f;
              (*pa_out)(i, j) += detail(i, j) * noise_amp * 0.02f * range * mask_v;
            }
        }
      },
      node.get_config_ref()->hmap_transform_mode_cpu);

  p_out->smooth_overlap_buffers();

  post_process_heightmap(node, *p_out, p_in);
}

} // namespace hesiod
