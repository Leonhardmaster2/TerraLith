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

void setup_foothills_transition_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mountains");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "plains");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "blend_mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<FloatAttribute>("transition_width", "Transition Width", 0.3f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("foothill_scale", "Foothill Scale", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("roughness_decay", "Roughness Decay", 0.7f, 0.f, 1.f);
  node.add_attr<IntAttribute>("octaves", "Detail Octaves", 6, 1, 16);
  node.add_attr<FloatAttribute>("noise_amp", "Noise Amplitude", 0.15f, 0.f, 0.5f);
  node.add_attr<FloatAttribute>("gamma", "Transition Gamma", 1.5f, 0.5f, 4.f);

  // attribute(s) order
  node.set_attr_ordered_key({"seed",
                             "_TEXT_Blending",
                             "transition_width",
                             "gamma",
                             "_TEXT_Foothills Detail",
                             "foothill_scale",
                             "roughness_decay",
                             "octaves",
                             "noise_amp"});

  setup_post_process_heightmap_attributes(node);
}

void compute_foothills_transition_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_mountains = node.get_value_ref<hmap::Heightmap>("mountains");
  hmap::Heightmap *p_plains = node.get_value_ref<hmap::Heightmap>("plains");

  if (!p_mountains || !p_plains)
    return;

  hmap::Heightmap *p_blend = node.get_value_ref<hmap::Heightmap>("blend_mask");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  uint  seed = node.get_attr<SeedAttribute>("seed");
  float transition_w = node.get_attr<FloatAttribute>("transition_width");
  float foothill_scale = node.get_attr<FloatAttribute>("foothill_scale");
  float roughness_decay = node.get_attr<FloatAttribute>("roughness_decay");
  int   octaves = node.get_attr<IntAttribute>("octaves");
  float noise_amp = node.get_attr<FloatAttribute>("noise_amp");
  float gamma = node.get_attr<FloatAttribute>("gamma");

  hmap::transform(
      {p_out, p_mountains, p_plains, p_blend},
      [seed, transition_w, foothill_scale, roughness_decay, octaves, noise_amp, gamma](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_mtn = p_arrays[1];
        hmap::Array *pa_plain = p_arrays[2];
        hmap::Array *pa_blend = p_arrays[3];

        // Generate blend factor if no mask is provided (based on mountain elevation)
        hmap::Array blend(shape);
        if (pa_blend)
        {
          blend = *pa_blend;
        }
        else
        {
          // Auto-blend: use mountain elevation as the blend factor
          float mmin = pa_mtn->min();
          float mmax = pa_mtn->max();
          float mrange = mmax - mmin;
          if (mrange < 1e-6f)
            mrange = 1.f;

          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
              blend(i, j) = ((*pa_mtn)(i, j) - mmin) / mrange;
        }

        // Apply gamma to transition
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
            blend(i, j) = std::pow(std::clamp(blend(i, j), 0.f, 1.f), gamma);

        // Generate foothill detail noise
        hmap::Array detail = hmap::noise_fbm(
            hmap::NoiseType::PERLIN,
            shape,
            {8.f, 8.f},
            seed,
            octaves,
            0.7f,
            0.5f,
            2.f,
            nullptr, nullptr, nullptr, nullptr,
            bbox);

        // Blend mountains with plains, adding foothill detail in the transition zone
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float t = blend(i, j);

            // Transition zone: highest detail where t is near transition_width
            float transition_factor = 1.f - std::abs(t - transition_w) / std::max(transition_w, 0.01f);
            transition_factor = std::clamp(transition_factor, 0.f, 1.f);

            // Roughness decreases from mountains to plains
            float local_roughness = t * roughness_decay + (1.f - t) * 0.1f;

            float mtn_v = (*pa_mtn)(i, j);
            float plain_v = (*pa_plain)(i, j);

            // Smooth blend
            float base = t * mtn_v + (1.f - t) * plain_v;

            // Add foothill detail noise in transition zone
            float foothill_noise = detail(i, j) * noise_amp * transition_factor *
                                   foothill_scale * local_roughness;

            (*pa_out)(i, j) = base + foothill_noise;
          }
      },
      node.get_config_ref()->hmap_transform_mode_cpu);

  p_out->smooth_overlap_buffers();

  post_process_heightmap(node, *p_out);
}

} // namespace hesiod
