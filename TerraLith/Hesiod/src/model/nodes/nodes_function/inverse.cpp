/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

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

void setup_inverse_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  setup_post_process_heightmap_attributes(node);
}

void compute_inverse_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    hmap::transform(
        {p_out, p_in},
        [](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_in = p_arrays[1];

          *pa_out = -*pa_in;
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    // post-process
    post_process_heightmap(node, *p_out);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_inverse_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  struct { uint32_t width; uint32_t height; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_in  = p_in->tiles[i];
    auto &tile_out = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile_in.shape.x);
    pc.height = static_cast<uint32_t>(tile_in.shape.y);

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_in.vector.data(), buf_size);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    std::vector<VulkanBuffer *> buffers = {&input_buf, &output_buf};
    gp.dispatch("inverse", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  post_process_heightmap(node, *p_out);
  return true;
}
#endif

} // namespace hesiod
