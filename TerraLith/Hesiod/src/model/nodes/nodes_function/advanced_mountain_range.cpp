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

void setup_advanced_mountain_range_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dx");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dy");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "envelope");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "out", CONFIG(node));

  // attribute(s)
  std::vector<float> kw = {4.f, 4.f};
  node.add_attr<WaveNbAttribute>("kw", "Spatial Frequency", kw, 0.f, FLT_MAX, true);
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<IntAttribute>("octaves", "Octaves", 8, 0, 32);
  node.add_attr<FloatAttribute>("persistence", "Persistence", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("lacunarity", "Lacunarity", 2.f, 0.01f, 4.f);
  node.add_attr<FloatAttribute>("ridge_sharpness", "Ridge Sharpness", 2.f, 0.1f, 8.f);
  node.add_attr<FloatAttribute>("ridge_offset", "Ridge Offset", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("peak_elevation", "Peak Elevation", 0.8f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("spine_kw", "Spine Frequency", 2.f, 0.01f, FLT_MAX);
  node.add_attr<FloatAttribute>("spine_amp", "Spine Amplitude", 0.3f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("talus_angle", "Talus Angle", 35.f, 10.f, 80.f, "{:.0f}");
  node.add_attr<FloatAttribute>("erosion_amt", "Erosion Amount", 0.2f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("snow_line", "Snow Line", 0.7f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("foothill_width", "Foothill Width", 0.4f, 0.f, 1.f);
  node.add_attr<Vec2FloatAttribute>("center", "center");

  // attribute(s) order
  node.set_attr_ordered_key({"_GROUPBOX_BEGIN_Main Parameters",
                             "kw",
                             "seed",
                             "octaves",
                             "persistence",
                             "lacunarity",
                             "_TEXT_Ridge Structure",
                             "ridge_sharpness",
                             "ridge_offset",
                             "peak_elevation",
                             "_TEXT_Spine Control",
                             "spine_kw",
                             "spine_amp",
                             "_TEXT_Surface Detail",
                             "talus_angle",
                             "erosion_amt",
                             "snow_line",
                             "foothill_width",
                             "_TEXT_Position",
                             "center",
                             "_GROUPBOX_END_"});

  setup_post_process_heightmap_attributes(node);
}

void compute_advanced_mountain_range_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("out");

  float ridge_sharpness = node.get_attr<FloatAttribute>("ridge_sharpness");
  float peak_elev = node.get_attr<FloatAttribute>("peak_elevation");
  float spine_kw = node.get_attr<FloatAttribute>("spine_kw");
  float spine_amp = node.get_attr<FloatAttribute>("spine_amp");
  float erosion_amt = node.get_attr<FloatAttribute>("erosion_amt");
  float foothill_width = node.get_attr<FloatAttribute>("foothill_width");

  // Use ridged noise as a base, then compose with a spine envelope
  hmap::transform(
      {p_out, p_dx, p_dy},
      [&node, ridge_sharpness, spine_kw, spine_amp](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_dx = p_arrays[1];
        hmap::Array *pa_dy = p_arrays[2];

        // Ridged noise base layer
        *pa_out = hmap::gpu::noise_fbm(
            hmap::NoiseType::PERLIN,
            shape,
            node.get_attr<WaveNbAttribute>("kw"),
            node.get_attr<SeedAttribute>("seed"),
            node.get_attr<IntAttribute>("octaves"),
            0.7f,
            node.get_attr<FloatAttribute>("persistence"),
            node.get_attr<FloatAttribute>("lacunarity"),
            nullptr, // no control
            pa_dx,
            pa_dy,
            nullptr,
            bbox);

        // Apply ridging: abs(2*x - 1) inverted for peaks
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float v = (*pa_out)(i, j);
            // ridge transform
            v = 1.f - std::pow(std::abs(2.f * v - 1.f), 1.f / ridge_sharpness);
            (*pa_out)(i, j) = v;
          }

        // Apply spine modulation (a low-frequency directional envelope)
        hmap::Array spine = hmap::noise(hmap::NoiseType::PERLIN,
                                        shape,
                                        {spine_kw, spine_kw * 0.3f},
                                        node.get_attr<SeedAttribute>("seed") + 1000,
                                        nullptr,
                                        nullptr,
                                        nullptr,
                                        bbox);

        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float s = std::clamp(spine(i, j) * spine_amp + 0.5f, 0.f, 1.f);
            (*pa_out)(i, j) *= s;
          }
      },
      node.get_config_ref()->hmap_transform_mode_gpu);

  p_out->remap(0.f, peak_elev);

  // Simulate simple foothill falloff
  if (foothill_width > 0.f)
  {
    float gamma = 1.f + foothill_width * 2.f;
    hmap::transform(*p_out,
                    [gamma](hmap::Array &x) { x = hmap::pow(x, gamma); });
  }

  // Simple erosion simulation (smooth fill)
  if (erosion_amt > 0.01f)
  {
    int ir = static_cast<int>(erosion_amt * 10.f);
    if (ir > 0)
      p_out->smooth_overlap_buffers();
  }

  // post-process
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);
}

} // namespace hesiod
