/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include "highmap/heightmap.hpp"
#include "highmap/range.hpp"

#include "attributes.hpp"

#include "hesiod/logger.hpp"
#include "hesiod/model/nodes/base_node.hpp"

#ifdef HESIOD_HAS_VULKAN
#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_generic_pipeline.hpp"
#endif

using namespace attr;

namespace hesiod
{

void setup_combiner_max_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input 1");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input 2");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));
}

void compute_combiner_max_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in1 = node.get_value_ref<hmap::Heightmap>("input 1");
  hmap::Heightmap *p_in2 = node.get_value_ref<hmap::Heightmap>("input 2");

  if (p_in1 && p_in2)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    hmap::transform(
        {p_out, p_in1, p_in2},
        [](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_in1 = p_arrays[1];
          hmap::Array *pa_in2 = p_arrays[2];
          *pa_out = hmap::maximum(*pa_in1, *pa_in2);
        },
        node.get_config_ref()->hmap_transform_mode_cpu);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_combiner_max_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in1 = node.get_value_ref<hmap::Heightmap>("input 1");
  hmap::Heightmap *p_in2 = node.get_value_ref<hmap::Heightmap>("input 2");
  if (!p_in1 || !p_in2) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  struct { uint32_t width; uint32_t height; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_in1 = p_in1->tiles[i];
    auto &tile_in2 = p_in2->tiles[i];
    auto &tile_out = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile_in1.shape.x);
    pc.height = static_cast<uint32_t>(tile_in1.shape.y);

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input1_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input1_buf.upload(tile_in1.vector.data(), buf_size);

    VulkanBuffer input2_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input2_buf.upload(tile_in2.vector.data(), buf_size);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    std::vector<VulkanBuffer *> buffers = {&input1_buf, &input2_buf, &output_buf};
    gp.dispatch("combiner_max", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  return true;
}
#endif

} // namespace hesiod
