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

void setup_alpine_peaks_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dx");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dy");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "envelope");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "out", CONFIG(node));

  // attribute(s)
  std::vector<float> kw = {3.f, 3.f};
  node.add_attr<WaveNbAttribute>("kw", "Spatial Frequency", kw, 0.f, FLT_MAX, true);
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<IntAttribute>("octaves", "Octaves", 10, 0, 32);
  node.add_attr<FloatAttribute>("peak_sharpness", "Peak Sharpness", 3.f, 0.5f, 10.f);
  node.add_attr<FloatAttribute>("ridge_persistence", "Ridge Persistence", 0.6f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("elevation", "Elevation", 0.85f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("arete_strength", "Arete Strength", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("cirque_depth", "Cirque Depth", 0.15f, 0.f, 0.5f);
  node.add_attr<FloatAttribute>("snow_cap_line", "Snow Cap Line", 0.75f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("talus_angle", "Talus Angle", 35.f, 10.f, 80.f, "{:.0f}");
  node.add_attr<Vec2FloatAttribute>("center", "center");

  // attribute(s) order
  node.set_attr_ordered_key({"_GROUPBOX_BEGIN_Main Parameters",
                             "kw",
                             "seed",
                             "octaves",
                             "elevation",
                             "_TEXT_Peak Structure",
                             "peak_sharpness",
                             "ridge_persistence",
                             "arete_strength",
                             "_TEXT_Alpine Features",
                             "cirque_depth",
                             "snow_cap_line",
                             "talus_angle",
                             "_TEXT_Position",
                             "center",
                             "_GROUPBOX_END_"});

  setup_post_process_heightmap_attributes(node);
}

void compute_alpine_peaks_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("out");

  float peak_sharp = node.get_attr<FloatAttribute>("peak_sharpness");
  float ridge_pers = node.get_attr<FloatAttribute>("ridge_persistence");
  float elevation = node.get_attr<FloatAttribute>("elevation");
  float arete_str = node.get_attr<FloatAttribute>("arete_strength");
  float cirque_depth = node.get_attr<FloatAttribute>("cirque_depth");

  hmap::transform(
      {p_out, p_dx, p_dy},
      [&node, peak_sharp, ridge_pers, arete_str, cirque_depth](
          std::vector<hmap::Array *> p_arrays,
          hmap::Vec2<int>            shape,
          hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_dx = p_arrays[1];
        hmap::Array *pa_dy = p_arrays[2];

        // Base ridged noise for mountain structure
        *pa_out = hmap::gpu::noise_fbm(
            hmap::NoiseType::PERLIN,
            shape,
            node.get_attr<WaveNbAttribute>("kw"),
            node.get_attr<SeedAttribute>("seed"),
            node.get_attr<IntAttribute>("octaves"),
            0.7f,
            ridge_pers,
            2.f,
            nullptr,
            pa_dx,
            pa_dy,
            nullptr,
            bbox);

        // Apply ridging for sharp peaks
        for (int j = 0; j < shape.y; j++)
          for (int i = 0; i < shape.x; i++)
          {
            float v = (*pa_out)(i, j);
            // Multi-layer ridging
            v = 1.f - std::abs(2.f * v - 1.f);
            v = std::pow(v, 1.f / peak_sharp);
            (*pa_out)(i, j) = v;
          }

        // Add arete ridges (second noise layer at different frequency)
        if (arete_str > 0.01f)
        {
          hmap::Array arete = hmap::gpu::noise_fbm(
              hmap::NoiseType::PERLIN,
              shape,
              {node.get_attr<WaveNbAttribute>("kw")[0] * 1.5f,
               node.get_attr<WaveNbAttribute>("kw")[1] * 1.5f},
              node.get_attr<SeedAttribute>("seed") + 500,
              6,
              0.7f,
              0.5f,
              2.f,
              nullptr, nullptr, nullptr, nullptr,
              bbox);

          for (int j = 0; j < shape.y; j++)
            for (int i = 0; i < shape.x; i++)
            {
              float a = 1.f - std::abs(2.f * arete(i, j) - 1.f);
              a = std::pow(a, 2.f);
              (*pa_out)(i, j) += a * arete_str * 0.3f;
            }
        }

        // Carve cirques (bowl-shaped depressions near high points)
        if (cirque_depth > 0.01f)
        {
          for (int j = 2; j < shape.y - 2; j++)
            for (int i = 2; i < shape.x - 2; i++)
            {
              float v = (*pa_out)(i, j);
              // High elevation concavity = cirque candidate
              if (v > 0.6f)
              {
                float laplacian = ((*pa_out)(i + 1, j) + (*pa_out)(i - 1, j) +
                                   (*pa_out)(i, j + 1) + (*pa_out)(i, j - 1) -
                                   4.f * v);
                // Only carve concave areas
                if (laplacian > 0.f)
                  (*pa_out)(i, j) -= laplacian * cirque_depth;
              }
            }
        }
      },
      node.get_config_ref()->hmap_transform_mode_gpu);

  p_out->remap(0.f, elevation);

  // post-process
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);
}

} // namespace hesiod
