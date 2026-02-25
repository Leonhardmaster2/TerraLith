/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>
#include <cmath>

#include "highmap/opencl/gpu_opencl.hpp"
#include "highmap/primitives.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/post_process.hpp"

#ifdef HESIOD_HAS_VULKAN
#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/vulkan_generic_pipeline.hpp"
#endif

using namespace attr;

namespace hesiod
{

void setup_gabor_wave_fbm_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dx");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dy");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "control");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "envelope");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "angle");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<WaveNbAttribute>("kw", "Spatial Frequency");
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<FloatAttribute>("angle", "angle", 0.f, -180.f, 180.f);
  node.add_attr<FloatAttribute>("angle_spread_ratio",
                                "angle_spread_ratio",
                                1.f,
                                0.f,
                                1.f);
  node.add_attr<IntAttribute>("octaves", "Octaves", 8, 0, 32);
  node.add_attr<FloatAttribute>("weight", "Weight", 0.7f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("persistence", "Persistence", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("lacunarity", "Lacunarity", 2.f, 0.01f, 4.f);

  // NOTE: Vulkan GPU toggle is provided by node_settings_widget ("Enable GPU
  // Compute" checkbox) for all DECLARE_NODE_VULKAN nodes — no manual "GPU"
  // attribute needed here.

  // attribute(s) order
  node.set_attr_ordered_key({"_GROUPBOX_BEGIN_Main Parameters",
                             "_TEXT_Frequency",
                             "kw",
                             "_TEXT_Orientation",
                             "angle",
                             "angle_spread_ratio",
                             "_TEXT_FBM layers",
                             "seed",
                             "octaves",
                             "weight",
                             "persistence",
                             "lacunarity",
                             "_GROUPBOX_END_"});

  setup_post_process_heightmap_attributes(node);
}

void compute_gabor_wave_fbm_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_ctrl = node.get_value_ref<hmap::Heightmap>("control");
  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Heightmap *p_angle = node.get_value_ref<hmap::Heightmap>("angle");

  hmap::transform(
      {p_out, p_ctrl, p_dx, p_dy, p_angle},
      [&node](std::vector<hmap::Array *> p_arrays,
              hmap::Vec2<int>            shape,
              hmap::Vec4<float>          bbox)
      {
        hmap::Array *pa_out = p_arrays[0];
        hmap::Array *pa_ctrl = p_arrays[1];
        hmap::Array *pa_dx = p_arrays[2];
        hmap::Array *pa_dy = p_arrays[3];
        hmap::Array *pa_angle = p_arrays[4];

        hmap::Array angle_deg(shape, node.get_attr<FloatAttribute>("angle"));

        if (pa_angle)
          angle_deg += (*pa_angle) * 180.f / M_PI;

        *pa_out = hmap::gpu::gabor_wave_fbm(
            shape,
            node.get_attr<WaveNbAttribute>("kw"),
            node.get_attr<SeedAttribute>("seed"),
            angle_deg,
            node.get_attr<FloatAttribute>("angle_spread_ratio"),
            node.get_attr<IntAttribute>("octaves"),
            node.get_attr<FloatAttribute>("weight"),
            node.get_attr<FloatAttribute>("persistence"),
            node.get_attr<FloatAttribute>("lacunarity"),
            pa_ctrl,
            pa_dx,
            pa_dy,
            bbox);
      },
      node.get_config_ref()->hmap_transform_mode_gpu);

  // post-process
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);
}

#ifdef HESIOD_HAS_VULKAN

// Push constants layout — must match gabor_wave_fbm.comp exactly
struct GaborWaveFbmPushConstants
{
  uint32_t width;
  uint32_t height;
  float    kw_x;
  float    kw_y;
  uint32_t seed;
  float    angle_spread_ratio;
  int32_t  octaves;
  float    weight;
  float    persistence;
  float    lacunarity;
  float    bbox_x;
  float    bbox_y;
  float    bbox_z;
  float    bbox_w;
  int32_t  has_ctrl;
  int32_t  has_noise_x;
  int32_t  has_noise_y;
};

static_assert(sizeof(GaborWaveFbmPushConstants) == 68,
              "GaborWaveFbm push constants must be exactly 68 bytes");

bool compute_gabor_wave_fbm_node_vulkan(BaseNode &node)
{
  // Note: the caller (BaseNode::compute) already checks vulkan_enabled_
  // before invoking this function, so no manual GPU toggle check needed.

  // Check Vulkan availability
  auto &vk_ctx = VulkanContext::instance();
  if (!vk_ctx.is_ready())
    return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready())
    return false;

  // V1 limitation: fall back to CPU/OpenCL if dx, dy, or control are connected
  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_ctrl = node.get_value_ref<hmap::Heightmap>("control");

  if (p_dx || p_dy || p_ctrl)
    return false;

  Logger::log()->trace(
      "compute_gabor_wave_fbm_node_vulkan: Vulkan path for node [{}]/[{}]",
      node.get_label(),
      node.get_id());

  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Heightmap *p_angle_hmap = node.get_value_ref<hmap::Heightmap>("angle");

  // Extract attributes once
  auto  kw = node.get_attr<WaveNbAttribute>("kw");
  uint  seed = node.get_attr<SeedAttribute>("seed");
  float angle_attr = node.get_attr<FloatAttribute>("angle");
  float angle_spread_ratio = node.get_attr<FloatAttribute>("angle_spread_ratio");
  int   octaves = node.get_attr<IntAttribute>("octaves");
  float weight = node.get_attr<FloatAttribute>("weight");
  float persistence = node.get_attr<FloatAttribute>("persistence");
  float lacunarity = node.get_attr<FloatAttribute>("lacunarity");

  // Profiling accumulators
  using Clock = std::chrono::high_resolution_clock;
  double phase_a_ms = 0.0; // buffer alloc
  double phase_b_ms = 0.0; // host→device (angle upload)
  double phase_c_ms = 0.0; // GPU execution
  double phase_d_ms = 0.0; // device→host (download)
  auto   total_start = Clock::now();

  size_t ntiles = p_out->get_ntiles();

  // Dispatch Vulkan compute per tile via the generic pipeline
  for (size_t ti = 0; ti < ntiles; ++ti)
  {
    auto &tile = p_out->tiles[ti];

    // Build per-pixel angle array (degrees) for this tile
    hmap::Array angle_deg(tile.shape, angle_attr);

    if (p_angle_hmap)
      angle_deg += p_angle_hmap->tiles[ti] * (180.f / static_cast<float>(M_PI));

    GaborWaveFbmPushConstants params{};
    params.width = static_cast<uint32_t>(tile.shape.x);
    params.height = static_cast<uint32_t>(tile.shape.y);
    params.kw_x = kw[0];
    params.kw_y = kw[1];
    params.seed = seed;
    params.angle_spread_ratio = angle_spread_ratio;
    params.octaves = octaves;
    params.weight = weight;
    params.persistence = persistence;
    params.lacunarity = lacunarity;
    params.bbox_x = tile.bbox.a; // xmin
    params.bbox_y = tile.bbox.c; // ymin
    params.bbox_z = tile.bbox.b; // xmax
    params.bbox_w = tile.bbox.d; // ymax
    params.has_ctrl = 0;
    params.has_noise_x = 0;
    params.has_noise_y = 0;

    VkDeviceSize pixel_count =
        static_cast<VkDeviceSize>(tile.shape.x) * tile.shape.y;
    VkDeviceSize buf_size = pixel_count * sizeof(float);

    // Phase A: Buffer allocation (output SSBO + angle SSBO)
    auto t0 = Clock::now();
    VulkanBuffer output_buf(buf_size,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    VulkanBuffer angle_buf(buf_size,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    auto t1 = Clock::now();
    phase_a_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Phase B: Host→Device upload (angle buffer)
    auto t2 = Clock::now();
    angle_buf.upload(angle_deg.vector.data(), buf_size);
    auto t3 = Clock::now();
    phase_b_ms += std::chrono::duration<double, std::milli>(t3 - t2).count();

    // Phase C: GPU dispatch
    uint32_t group_x = (params.width + 15) / 16;
    uint32_t group_y = (params.height + 15) / 16;

    auto t4 = Clock::now();
    std::vector<VulkanBuffer *> buffers = {&output_buf, &angle_buf};
    gp.dispatch("gabor_wave_fbm",
                &params,
                sizeof(GaborWaveFbmPushConstants),
                buffers,
                group_x,
                group_y);
    auto t5 = Clock::now();
    phase_c_ms += std::chrono::duration<double, std::milli>(t5 - t4).count();

    // Phase D: Device→Host download
    auto t6 = Clock::now();
    output_buf.download(tile.vector.data(), buf_size);
    auto t7 = Clock::now();
    phase_d_ms += std::chrono::duration<double, std::milli>(t7 - t6).count();
  }

  auto   total_end = Clock::now();
  double total_ms =
      std::chrono::duration<double, std::milli>(total_end - total_start).count();

  Logger::log()->info("═══ VULKAN PROFILING: GaborWaveFbm [{}] ═══", node.get_id());
  Logger::log()->info("  Tiles: {}, Resolution per tile: {}x{}",
                      ntiles,
                      ntiles > 0 ? p_out->tiles[0].shape.x : 0,
                      ntiles > 0 ? p_out->tiles[0].shape.y : 0);
  Logger::log()->info("  Phase A (buffer alloc):    {:7.2f} ms  [{:5.1f}%]",
                      phase_a_ms,
                      100.0 * phase_a_ms / total_ms);
  Logger::log()->info("  Phase B (host→device):     {:7.2f} ms  [{:5.1f}%]",
                      phase_b_ms,
                      100.0 * phase_b_ms / total_ms);
  Logger::log()->info("  Phase C (GPU dispatch):    {:7.2f} ms  [{:5.1f}%]",
                      phase_c_ms,
                      100.0 * phase_c_ms / total_ms);
  Logger::log()->info("  Phase D (device→host):     {:7.2f} ms  [{:5.1f}%]",
                      phase_d_ms,
                      100.0 * phase_d_ms / total_ms);
  Logger::log()->info(
      "  Unaccounted (buf dealloc): {:7.2f} ms  [{:5.1f}%]",
      total_ms - (phase_a_ms + phase_b_ms + phase_c_ms + phase_d_ms),
      100.0 *
          (total_ms - (phase_a_ms + phase_b_ms + phase_c_ms + phase_d_ms)) /
          total_ms);
  Logger::log()->info("  TOTAL:                     {:7.2f} ms", total_ms);

  // Diagnostic: compare first 10 values of first tile for CPU/GPU parity check
  if (ntiles > 0)
  {
    auto &tile = p_out->tiles[0];
    int   n = std::min(10, tile.shape.x * tile.shape.y);
    std::string vals;
    for (int i = 0; i < n; ++i)
    {
      if (i > 0) vals += ", ";
      vals += std::format("{:.6f}", tile.vector[i]);
    }
    Logger::log()->info("  GPU first {} values: [{}]", n, vals);
  }

  Logger::log()->info("═══════════════════════════════════════════");

  // Post-processing (CPU)
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);

  return true;
}
#endif

} // namespace hesiod
