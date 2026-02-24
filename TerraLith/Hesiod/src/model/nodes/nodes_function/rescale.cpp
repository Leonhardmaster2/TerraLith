/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/range.hpp"

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

void setup_rescale_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("scaling", "scaling", 1.f, 0.0001f, FLT_MAX, "{:.4f}");
  node.add_attr<BoolAttribute>("centered", "centered", false);

  // attribute(s) order
  node.set_attr_ordered_key({"scaling", "centered"});
}

void compute_rescale_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // copy the input heightmap
    *p_out = *p_in;

    float vref = 0.f;

    if (node.get_attr<BoolAttribute>("centered"))
      vref = p_out->mean();

    hmap::transform(
        {p_out},
        [&node, vref](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];

          hmap::rescale(*pa_out, node.get_attr<FloatAttribute>("scaling"), vref);
        },
        node.get_config_ref()->hmap_transform_mode_cpu);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_rescale_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  // Copy input to output first (need global mean if centered)
  *p_out = *p_in;

  float scaling  = node.get_attr<FloatAttribute>("scaling");
  bool  centered = node.get_attr<BoolAttribute>("centered");
  float vref     = centered ? p_out->mean() : 0.f;

  struct { uint32_t width; uint32_t height; float scaling; float vref; int32_t centered; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_in  = p_in->tiles[i];
    auto &tile_out = p_out->tiles[i];

    pc.width    = static_cast<uint32_t>(tile_in.shape.x);
    pc.height   = static_cast<uint32_t>(tile_in.shape.y);
    pc.scaling  = scaling;
    pc.vref     = vref;
    pc.centered = centered ? 1 : 0;

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_in.vector.data(), buf_size);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VulkanBuffer *> buffers = {&input_buf, &output_buf};
    gp.dispatch("rescale", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  return true;
}
#endif

} // namespace hesiod
