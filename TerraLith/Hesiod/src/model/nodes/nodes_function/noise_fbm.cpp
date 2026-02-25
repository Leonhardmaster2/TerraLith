/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>

#include "highmap/opencl/gpu_opencl.hpp"
#include "highmap/primitives.hpp"

#include "attributes.hpp"

#include "hesiod/app/enum_mappings.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/post_process.hpp"

#ifdef HESIOD_HAS_VULKAN
#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/vulkan_generic_pipeline.hpp"
#include "hesiod/gpu/vulkan/vulkan_noise_pipeline.hpp"
#endif

using namespace attr;

namespace hesiod
{

void setup_noise_fbm_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dx");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "dy");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "control");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "envelope");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<EnumAttribute>("noise_type", "Type", enum_mappings.noise_type_map_fbm);
  node.add_attr<WaveNbAttribute>("kw", "Spatial Frequency");
  node.add_attr<SeedAttribute>("seed", "Seed");
  node.add_attr<IntAttribute>("octaves", "Octaves", 8, 0, 32);
  node.add_attr<FloatAttribute>("weight", "Weight", 0.7f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("persistence", "Persistence", 0.5f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("lacunarity", "Lacunarity", 2.f, 0.01f, 4.f);

  // NOTE: Vulkan GPU toggle is provided by node_settings_widget ("Enable GPU
  // Compute" checkbox) for all DECLARE_NODE_VULKAN nodes — no manual "GPU"
  // attribute needed here.

  // attribute(s) order
  node.set_attr_ordered_key({"noise_type",
                             "kw",
                             "seed",
                             "octaves",
                             "weight",
                             "persistence",
                             "lacunarity"});

  setup_post_process_heightmap_attributes(node);
}

void compute_noise_fbm_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  // base noise function
  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_ctrl = node.get_value_ref<hmap::Heightmap>("control");
  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  // When GPU compute is enabled (and Vulkan failed, falling back here),
  // try OpenCL; otherwise use pure CPU.
  if (node.is_vulkan_enabled())
  {
    hmap::transform(
        {p_out, p_ctrl, p_dx, p_dy},
        [&node](std::vector<hmap::Array *> p_arrays,
                hmap::Vec2<int>            shape,
                hmap::Vec4<float>          bbox)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_ctrl = p_arrays[1];
          hmap::Array *pa_dx = p_arrays[2];
          hmap::Array *pa_dy = p_arrays[3];

          *pa_out = hmap::gpu::noise_fbm(
              (hmap::NoiseType)node.get_attr<EnumAttribute>("noise_type"),
              shape,
              node.get_attr<WaveNbAttribute>("kw"),
              node.get_attr<SeedAttribute>("seed"),
              node.get_attr<IntAttribute>("octaves"),
              node.get_attr<FloatAttribute>("weight"),
              node.get_attr<FloatAttribute>("persistence"),
              node.get_attr<FloatAttribute>("lacunarity"),
              pa_ctrl,
              pa_dx,
              pa_dy,
              nullptr,
              bbox);
        },
        node.get_config_ref()->hmap_transform_mode_gpu);
  }
  else
  {
    hmap::transform(
        {p_out, p_ctrl, p_dx, p_dy},
        [&node](std::vector<hmap::Array *> p_arrays,
                hmap::Vec2<int>            shape,
                hmap::Vec4<float>          bbox)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_ctrl = p_arrays[1];
          hmap::Array *pa_dx = p_arrays[2];
          hmap::Array *pa_dy = p_arrays[3];

          *pa_out = hmap::noise_fbm(
              (hmap::NoiseType)node.get_attr<EnumAttribute>("noise_type"),
              shape,
              node.get_attr<WaveNbAttribute>("kw"),
              node.get_attr<SeedAttribute>("seed"),
              node.get_attr<IntAttribute>("octaves"),
              node.get_attr<FloatAttribute>("weight"),
              node.get_attr<FloatAttribute>("persistence"),
              node.get_attr<FloatAttribute>("lacunarity"),
              pa_ctrl,
              pa_dx,
              pa_dy,
              nullptr,
              bbox);
        },
        node.get_config_ref()->hmap_transform_mode_cpu);
  }

  // post-process
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);
}

#ifdef HESIOD_HAS_VULKAN
bool compute_noise_fbm_node_vulkan(BaseNode &node)
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

  // V1 limitation: fall back to CPU/OpenCL if optional inputs are connected
  hmap::Heightmap *p_dx = node.get_value_ref<hmap::Heightmap>("dx");
  hmap::Heightmap *p_dy = node.get_value_ref<hmap::Heightmap>("dy");
  hmap::Heightmap *p_ctrl = node.get_value_ref<hmap::Heightmap>("control");

  if (p_dx || p_dy || p_ctrl)
    return false;

  Logger::log()->trace("compute_noise_fbm_node_vulkan: Vulkan path for node [{}]/[{}]",
                       node.get_label(),
                       node.get_id());

  hmap::Heightmap *p_env = node.get_value_ref<hmap::Heightmap>("envelope");
  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  // Extract attributes once
  auto  kw = node.get_attr<WaveNbAttribute>("kw");
  uint  seed = node.get_attr<SeedAttribute>("seed");
  int   octaves = node.get_attr<IntAttribute>("octaves");
  float weight = node.get_attr<FloatAttribute>("weight");
  float persistence = node.get_attr<FloatAttribute>("persistence");
  float lacunarity = node.get_attr<FloatAttribute>("lacunarity");
  int   noise_type = node.get_attr<EnumAttribute>("noise_type");

  // ── Profiling accumulators ─────────────────────────────────────────
  using Clock = std::chrono::high_resolution_clock;
  double phase_a_ms = 0.0; // buffer alloc (vkCreateBuffer + vkAllocateMemory)
  double phase_b_ms = 0.0; // host→device (upload — trivial for noise, but measured)
  double phase_c_ms = 0.0; // GPU execution (descriptor setup + submit + wait)
  double phase_d_ms = 0.0; // device→host (download)
  auto   total_start = Clock::now();

  size_t ntiles = p_out->get_ntiles();

  // NOTE: VulkanBuffer is allocated + freed PER TILE (vkCreateBuffer +
  // vkAllocateMemory + vkDestroyBuffer + vkFreeMemory).  Descriptor pool
  // and command buffer + fence are also created/destroyed per-dispatch
  // inside VulkanGenericPipeline::dispatch().  This is the likely
  // bottleneck — not the GPU shader itself.

  // Dispatch Vulkan compute per tile via the generic pipeline
  for (size_t i = 0; i < ntiles; ++i)
  {
    auto &tile = p_out->tiles[i];

    NoiseFbmPushConstants params{};
    params.width = static_cast<uint32_t>(tile.shape.x);
    params.height = static_cast<uint32_t>(tile.shape.y);
    params.kw_x = kw[0];
    params.kw_y = kw[1];
    params.seed = seed;
    params.octaves = octaves;
    params.weight = weight;
    params.persistence = persistence;
    params.lacunarity = lacunarity;
    params.noise_type = noise_type;
    params.bbox_x = tile.bbox.a; // xmin
    params.bbox_y = tile.bbox.c; // ymin
    params.bbox_z = tile.bbox.b; // xmax
    params.bbox_w = tile.bbox.d; // ymax

    VkDeviceSize buf_size =
        static_cast<VkDeviceSize>(tile.shape.x) * tile.shape.y * sizeof(float);

    // Phase A: Buffer allocation
    auto t0 = Clock::now();
    VulkanBuffer output_buf(buf_size,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    auto t1 = Clock::now();
    phase_a_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Phase B: Host→Device upload (noise_fbm has no input buffers, skip)
    // (measured as zero for this node — combiner nodes would show upload time)

    // Phase C: GPU dispatch (descriptor pool + set alloc + cmd buffer + submit + wait)
    uint32_t group_x = (params.width + 15) / 16;
    uint32_t group_y = (params.height + 15) / 16;

    auto t2 = Clock::now();
    std::vector<VulkanBuffer *> buffers = {&output_buf};
    gp.dispatch("noise_fbm",
                &params,
                sizeof(NoiseFbmPushConstants),
                buffers,
                group_x,
                group_y);
    auto t3 = Clock::now();
    phase_c_ms += std::chrono::duration<double, std::milli>(t3 - t2).count();

    // Phase D: Device→Host download
    auto t4 = Clock::now();
    output_buf.download(tile.vector.data(), buf_size);
    auto t5 = Clock::now();
    phase_d_ms += std::chrono::duration<double, std::milli>(t5 - t4).count();

    // (VulkanBuffer destructor runs here — vkDestroyBuffer + vkFreeMemory)
  }

  auto total_end = Clock::now();
  double total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();

  Logger::log()->info("═══ VULKAN PROFILING: NoiseFbm [{}] ═══", node.get_id());
  Logger::log()->info("  Tiles: {}, Resolution per tile: {}x{}",
                      ntiles,
                      ntiles > 0 ? p_out->tiles[0].shape.x : 0,
                      ntiles > 0 ? p_out->tiles[0].shape.y : 0);
  Logger::log()->info("  Phase A (buffer alloc):    {:7.2f} ms  [{:5.1f}%]",
                      phase_a_ms, 100.0 * phase_a_ms / total_ms);
  Logger::log()->info("  Phase B (host→device):     {:7.2f} ms  [{:5.1f}%]  (no input bufs)",
                      phase_b_ms, 100.0 * phase_b_ms / total_ms);
  Logger::log()->info("  Phase C (GPU dispatch):    {:7.2f} ms  [{:5.1f}%]  "
                      "(includes desc pool + cmd buf + fence per tile!)",
                      phase_c_ms, 100.0 * phase_c_ms / total_ms);
  Logger::log()->info("  Phase D (device→host):     {:7.2f} ms  [{:5.1f}%]",
                      phase_d_ms, 100.0 * phase_d_ms / total_ms);
  Logger::log()->info("  Unaccounted (buf dealloc): {:7.2f} ms  [{:5.1f}%]",
                      total_ms - (phase_a_ms + phase_b_ms + phase_c_ms + phase_d_ms),
                      100.0 * (total_ms - (phase_a_ms + phase_b_ms + phase_c_ms + phase_d_ms)) / total_ms);
  Logger::log()->info("  TOTAL:                     {:7.2f} ms", total_ms);
  Logger::log()->info("═══════════════════════════════════════════");

  // Post-processing (CPU)
  post_apply_enveloppe(node, *p_out, p_env);
  post_process_heightmap(node, *p_out);

  return true;
}
#endif

} // namespace hesiod
