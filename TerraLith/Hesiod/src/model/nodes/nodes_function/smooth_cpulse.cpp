/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/filters.hpp"
#include "highmap/opencl/gpu_opencl.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"
#include "hesiod/model/nodes/post_process.hpp"

#ifdef HESIOD_HAS_VULKAN
#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_generic_pipeline.hpp"
#endif

using namespace attr;

namespace hesiod
{

void setup_smooth_cpulse_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("radius", "radius", 0.05f, 0.f, 0.2f);

  // attribute(s) order
  node.set_attr_ordered_key({"radius"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_smooth_cpulse_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // prepare mask
    std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

    // copy the input heightmap
    *p_out = *p_in;

    int ir = std::max(1, (int)(node.get_attr<FloatAttribute>("radius") * p_out->shape.x));

    hmap::transform(
        {p_out, p_mask},
        [&ir](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_mask = p_arrays[1];

          hmap::gpu::smooth_cpulse(*pa_out, ir, pa_mask);
        },
        node.get_config_ref()->hmap_transform_mode_gpu);

    p_out->smooth_overlap_buffers();

    // post-process
    post_process_heightmap(node, *p_out, p_in);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_smooth_cpulse_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in)
    return false;

  // Fall back to CPU when a mask is connected (masking requires per-pixel
  // blending between original and blurred data that the separable GPU shaders
  // do not handle).
  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  if (p_mask)
    return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready())
    return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  float radius = node.get_attr<FloatAttribute>("radius");

  // Cap GPU path at radius <= 32 pixels (shared memory limit in the shader).
  int ir_check = std::max(1, (int)(radius * p_out->shape.x));
  if (ir_check > 32)
    return false;

  // Copy input to output first (matching CPU path)
  *p_out = *p_in;

  // Push constants shared by both horizontal and vertical passes
  struct
  {
    uint32_t width;
    uint32_t height;
    int32_t  radius;
    float    sigma;
  } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_out = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile_out.shape.x);
    pc.height = static_cast<uint32_t>(tile_out.shape.y);
    pc.radius = std::max(1, (int)(radius * tile_out.shape.x));
    // Sigma chosen so that the Gaussian covers the kernel radius well
    pc.sigma  = static_cast<float>(pc.radius) / 3.0f;
    if (pc.sigma < 0.5f)
      pc.sigma = 0.5f;

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    // Ping-pong buffers: input -> temp (H-blur) -> output (V-blur)
    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_out.vector.data(), buf_size);

    VulkanBuffer temp_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Pass 1: Horizontal blur — input_buf -> temp_buf
    {
      std::vector<VulkanBuffer *> buffers = {&input_buf, &temp_buf};
      gp.dispatch("blur_horizontal", &pc, sizeof(pc), buffers,
                  (pc.width + 255) / 256, pc.height);
    }

    // Pass 2: Vertical blur — temp_buf -> output_buf
    {
      std::vector<VulkanBuffer *> buffers = {&temp_buf, &output_buf};
      gp.dispatch("blur_vertical", &pc, sizeof(pc), buffers,
                  pc.width, (pc.height + 255) / 256);
    }

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  p_out->smooth_overlap_buffers();

  // post-process
  post_process_heightmap(node, *p_out, p_in);

  return true;
}
#endif

} // namespace hesiod
