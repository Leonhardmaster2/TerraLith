/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include "highmap/filters.hpp"

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

void setup_fold_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("k", "k", 0.1f, 0.f, 0.2f);
  node.add_attr<IntAttribute>("iterations", "iterations", 3, 1, 10);
}

void compute_fold_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // copy the input heightmap
    *p_out = *p_in;

    float hmin = p_out->min();
    float hmax = p_out->max();

    hmap::transform(
        {p_out},
        [&node, &hmin, &hmax](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];

          hmap::fold(*pa_out,
                     hmin,
                     hmax,
                     node.get_attr<IntAttribute>("iterations"),
                     node.get_attr<FloatAttribute>("k"));
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    p_out->remap(hmin, hmax);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_fold_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  *p_out = *p_in;

  float hmin = p_out->min();
  float hmax = p_out->max();

  struct { uint32_t width; uint32_t height; float hmin; float hmax; int32_t iterations; float k; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile = p_out->tiles[i];

    pc.width      = static_cast<uint32_t>(tile.shape.x);
    pc.height     = static_cast<uint32_t>(tile.shape.y);
    pc.hmin       = hmin;
    pc.hmax       = hmax;
    pc.iterations = node.get_attr<IntAttribute>("iterations");
    pc.k          = node.get_attr<FloatAttribute>("k");

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer data_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    data_buf.upload(tile.vector.data(), buf_size);

    std::vector<VulkanBuffer *> buffers = {&data_buf};
    gp.dispatch("fold", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    data_buf.download(tile.vector.data(), buf_size);
  }

  p_out->remap(hmin, hmax);
  return true;
}
#endif

} // namespace hesiod
