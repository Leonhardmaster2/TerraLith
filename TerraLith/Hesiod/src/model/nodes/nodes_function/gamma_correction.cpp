/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include "highmap/filters.hpp"
#include "highmap/heightmap.hpp"
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

void setup_gamma_correction_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "mask");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("gamma", "gamma", 2.f, 0.01f, 10.f);

  // attribute(s) order
  node.set_attr_ordered_key({"gamma"});

  setup_pre_process_mask_attributes(node);
  setup_post_process_heightmap_attributes(node, true);
}

void compute_gamma_correction_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // prepare mask
    std::shared_ptr<hmap::Heightmap> sp_mask = pre_process_mask(node, p_mask, *p_in);

    float hmin = p_in->min();
    float hmax = p_in->max();

    hmap::transform(
        {p_out, p_in, p_mask},
        [&node, hmin, hmax](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_in = p_arrays[1];
          hmap::Array *pa_mask = p_arrays[2];

          *pa_out = *pa_in;

          hmap::remap(*pa_out, 0.f, 1.f, hmin, hmax);
          hmap::gamma_correction(*pa_out,
                                 node.get_attr<FloatAttribute>("gamma"),
                                 pa_mask);
          hmap::remap(*pa_out, hmin, hmax, 0.f, 1.f);
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    // post-process
    post_process_heightmap(node, *p_out, p_in);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_gamma_correction_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  // Fall back to CPU if mask is connected
  hmap::Heightmap *p_mask = node.get_value_ref<hmap::Heightmap>("mask");
  if (p_mask) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  *p_out = *p_in;

  float hmin  = p_in->min();
  float hmax  = p_in->max();
  float gamma = node.get_attr<FloatAttribute>("gamma");

  struct { uint32_t width; uint32_t height; float gamma; float hmin; float hmax; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile.shape.x);
    pc.height = static_cast<uint32_t>(tile.shape.y);
    pc.gamma  = gamma;
    pc.hmin   = hmin;
    pc.hmax   = hmax;

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer data_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    data_buf.upload(tile.vector.data(), buf_size);

    std::vector<VulkanBuffer *> buffers = {&data_buf};
    gp.dispatch("gamma_correction", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    data_buf.download(tile.vector.data(), buf_size);
  }

  post_process_heightmap(node, *p_out, p_in);
  return true;
}
#endif

} // namespace hesiod
