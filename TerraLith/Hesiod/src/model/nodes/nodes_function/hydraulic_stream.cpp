/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>

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
  node.add_attr<BoolAttribute>("GPU", "GPU", HSD_DEFAULT_GPU_MODE);

  // attribute(s) order
  node.set_attr_ordered_key(
      {"c_erosion", "talus_ref", "radius", "clipping_ratio", "_SEPARATOR_", "GPU"});
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

  // Number of flow-accumulation relaxation iterations
  static constexpr uint32_t NUM_ITERATIONS = 50;

  // Track whether this is the first run (for baseline comparison)
  static bool first_run = true;

  Logger::log()->info("═══════════════════════════════════════════════");
  Logger::log()->info("═══ VULKAN SIMULATION ACTIVE ═══");
  Logger::log()->info("═══════════════════════════════════════════════");

  // --- Comparative diagnostic: CPU baseline on first run ---
  double cpu_baseline_ms = 0.0;

  if (first_run)
  {
    Logger::log()->info("[vulkan] Running CPU baseline for comparison...");

    // Run CPU path on first tile for baseline timing
    if (p_out->get_ntiles() > 0)
    {
      auto &tile_baseline = p_out->tiles[0];

      hmap::Array baseline_copy = tile_baseline;
      int ir_baseline = (int)(node.get_attr<FloatAttribute>("radius") * tile_baseline.shape.x);

      auto t_cpu_start = Clock::now();

      hmap::hydraulic_stream(baseline_copy,
                             c_erosion,
                             talus_ref,
                             nullptr,
                             nullptr,
                             nullptr,
                             ir_baseline,
                             clipping_ratio);

      auto t_cpu_end = Clock::now();

      cpu_baseline_ms =
          std::chrono::duration<double, std::milli>(t_cpu_end - t_cpu_start).count();

      Logger::log()->info("[vulkan] CPU/OpenCL Baseline (1 tile): {:.2f} ms", cpu_baseline_ms);
    }
  }

  // --- Vulkan dispatch per tile ---
  auto t_vulkan_total_start = Clock::now();

  double total_gpu_dispatch_ms = 0.0;
  double total_barrier_stall_ms = 0.0;

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

    auto metrics = ep.compute_erosion(tile_out.vector.data(),
                                      erosion_data,
                                      mask_data,
                                      w,
                                      h,
                                      c_erosion,
                                      talus_ref,
                                      clipping_ratio,
                                      NUM_ITERATIONS);

    total_gpu_dispatch_ms += metrics.total_gpu_dispatch_ms;
    total_barrier_stall_ms += metrics.memory_barrier_stall_ms;
  }

  auto t_vulkan_total_end = Clock::now();

  double vulkan_total_ms =
      std::chrono::duration<double, std::milli>(t_vulkan_total_end - t_vulkan_total_start)
          .count();

  // --- Performance logging ---
  double per_iter_avg = (NUM_ITERATIONS > 0) ? total_gpu_dispatch_ms / NUM_ITERATIONS : 0.0;

  Logger::log()->info("[vulkan] Iteration Count: {}", NUM_ITERATIONS);
  Logger::log()->info("[vulkan] Total GPU Dispatch Time: {:.2f} ms", total_gpu_dispatch_ms);
  Logger::log()->info("[vulkan] Per-Iteration Average: {:.2f} ms", per_iter_avg);
  Logger::log()->info("[vulkan] Memory Barrier Stall Time (Estimated): {:.2f} ms",
                      total_barrier_stall_ms);

  // --- Comparative diagnostic on first run ---
  if (first_run && cpu_baseline_ms > 0.0)
  {
    // Scale CPU baseline to estimate full-run time (all tiles * iterations)
    double cpu_estimated_total = cpu_baseline_ms * p_out->get_ntiles();
    double speedup = cpu_estimated_total / vulkan_total_ms;

    Logger::log()->info("[vulkan] ─────────────────────────────────────────");
    Logger::log()->info("[vulkan] CPU Baseline (estimated total): {:.2f} ms",
                        cpu_estimated_total);
    Logger::log()->info("[vulkan] Vulkan Total: {:.2f} ms", vulkan_total_ms);
    Logger::log()->info("[vulkan] Speedup: {:.1f}x faster", speedup);
    Logger::log()->info("[vulkan] ─────────────────────────────────────────");

    first_run = false;
  }

  Logger::log()->info("═══════════════════════════════════════════════");

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
