/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>
#include <cmath>

#include "highmap/erosion.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/post_process.hpp"

#ifdef HESIOD_HAS_VULKAN
#include "hesiod/gpu/vulkan/vulkan_erosion_pipeline.hpp"
#endif

using namespace attr;

namespace hesiod
{

void setup_hydraulic_stream_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "erosion", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("c_erosion", "c_erosion", 0.05f, 0.01f, 0.1f);
  node.add_attr<FloatAttribute>("talus_ref", "talus_ref", 0.1f, 0.01f, 10.f);
  node.add_attr<FloatAttribute>("radius", "radius", 0.f, 0.f, 0.05f);
  node.add_attr<FloatAttribute>("clipping_ratio", "clipping_ratio", 10.f, 0.1f, 100.f);

  // attribute(s) order
  // NOTE: Vulkan GPU toggle is provided by node_settings_widget ("Enable GPU
  // Compute" checkbox) for all DECLARE_NODE_VULKAN nodes — no manual "GPU"
  // attribute needed here.
  node.set_attr_ordered_key({"c_erosion", "talus_ref", "radius", "clipping_ratio"});
}

void compute_hydraulic_stream_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
    hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
    hmap::Heightmap *p_erosion_map = node.get_value_ref<hmap::Heightmap>("erosion");

    // copy the input heightmap
    *p_out = *p_in;

    int ir = (int)(node.get_attr<FloatAttribute>("radius") * p_out->shape.x);

    hmap::transform(
        {p_out, p_mask, p_erosion_map},
        [&node, &ir](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_mask = p_arrays[1];
          hmap::Array *pa_erosion_map = p_arrays[2];

          hmap::hydraulic_stream(*pa_out,
                                 pa_mask,
                                 node.get_attr<FloatAttribute>("c_erosion"),
                                 node.get_attr<FloatAttribute>("talus_ref"),
                                 nullptr,
                                 nullptr,
                                 pa_erosion_map,
                                 ir,
                                 node.get_attr<FloatAttribute>("clipping_ratio"));
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    p_out->smooth_overlap_buffers();

    p_erosion_map->smooth_overlap_buffers();
    p_erosion_map->remap();
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_hydraulic_stream_node_vulkan(BaseNode &node)
{
  using Clock = std::chrono::high_resolution_clock;

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in)
    return false;

  auto &ep = VulkanErosionPipeline::instance();
  if (!ep.is_ready())
    return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  hmap::Heightmap *p_erosion_map = node.get_value_ref<hmap::Heightmap>("erosion");

  float c_erosion = node.get_attr<FloatAttribute>("c_erosion");
  float talus_ref = node.get_attr<FloatAttribute>("talus_ref");
  float clipping_ratio = node.get_attr<FloatAttribute>("clipping_ratio");

  // Copy input to output first (matching CPU path)
  *p_out = *p_in;

  // Number of flow-accumulation relaxation iterations.
  // Each iteration propagates flow one cell downhill.  We need enough
  // iterations for flow to traverse the longest drainage path across
  // the tile.  max(width, height) covers most realistic terrain paths
  // (CPU's D-inf handles this in one topological pass, our iterative
  // relaxation needs explicit propagation steps).

  // --- Vulkan dispatch per tile ---
  // This mirrors the CPU path which also processes tiles independently via
  // hmap::transform.  Tile overlap regions are smoothed by
  // smooth_overlap_buffers() after all tiles complete.
  auto t_start = Clock::now();

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_out = p_out->tiles[i];

    uint32_t w = static_cast<uint32_t>(tile_out.shape.x);
    uint32_t h = static_cast<uint32_t>(tile_out.shape.y);

    // Get mask tile data (nullptr if no mask)
    const float *mask_data = nullptr;
    if (p_mask && i < p_mask->get_ntiles())
      mask_data = p_mask->tiles[i].vector.data();

    // Get erosion tile data
    float *erosion_data = nullptr;
    if (p_erosion_map && i < p_erosion_map->get_ntiles())
      erosion_data = p_erosion_map->tiles[i].vector.data();

    uint32_t num_iterations = std::max(w, h);

    ep.compute_erosion(tile_out.vector.data(),
                       erosion_data,
                       mask_data,
                       w,
                       h,
                       c_erosion,
                       talus_ref,
                       clipping_ratio,
                       num_iterations);

    // --- CPU-side sanitization: catch any NaN/Inf/extreme values that
    // survived the GPU pipeline (driver bugs, coherency glitches, etc.) ---
    {
      size_t num_pixels = static_cast<size_t>(w) * h;
      int    corrupt_count = 0;

      for (size_t px = 0; px < num_pixels; ++px)
      {
        float &val = tile_out.vector[px];
        if (std::isnan(val) || std::isinf(val) || val > 10000.0f || val < -10000.0f)
        {
          val = 0.0f;
          ++corrupt_count;
        }
      }

      if (erosion_data)
      {
        for (size_t px = 0; px < num_pixels; ++px)
        {
          float &val = erosion_data[px];
          if (std::isnan(val) || std::isinf(val) || val > 10000.0f || val < -10000.0f)
          {
            val = 0.0f;
            ++corrupt_count;
          }
        }
      }

      if (corrupt_count > 0)
        Logger::log()->warn("[vulkan] hydraulic_stream tile {}: sanitized {} corrupt "
                            "values (NaN/Inf/extreme) in GPU readback",
                            i,
                            corrupt_count);
    }
  }

  auto t_end = Clock::now();
  double total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

  Logger::log()->info("[vulkan] hydraulic_stream: {} tiles, {:.1f} ms total",
                      p_out->get_ntiles(),
                      total_ms);

  p_out->smooth_overlap_buffers();

  if (p_erosion_map)
  {
    p_erosion_map->smooth_overlap_buffers();
    p_erosion_map->remap();
  }

  return true;
}
#endif

} // namespace hesiod
