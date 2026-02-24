/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/filters.hpp"
#include "highmap/operator.hpp"

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

void setup_saturate_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("k_smoothing", "k_smoothing", 0.1f, 0.01, 1.f);
  node.add_attr<RangeAttribute>("range", "range");

  // link histogram for RangeAttribute
  setup_histogram_for_range_attribute(node, "range", "input");

  // attribute(s) order
  node.set_attr_ordered_key({"k_smoothing", "range"});

  setup_post_process_heightmap_attributes(node, true);
}

void compute_saturate_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    float hmin = p_in->min();
    float hmax = p_in->max();

    hmap::transform(
        {p_out, p_in},
        [&node, &hmin, &hmax](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_in = p_arrays[1];

          *pa_out = *pa_in;

          hmap::saturate(*pa_out,
                         node.get_attr<RangeAttribute>("range")[0],
                         node.get_attr<RangeAttribute>("range")[1],
                         hmin,
                         hmax,
                         node.get_attr<FloatAttribute>("k_smoothing"));
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    // post-process
    post_process_heightmap(node, *p_out, p_in);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_saturate_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");
  *p_out = *p_in;

  float hmin        = p_in->min();
  float hmax        = p_in->max();
  float range_min   = node.get_attr<RangeAttribute>("range")[0];
  float range_max   = node.get_attr<RangeAttribute>("range")[1];
  float k_smoothing = node.get_attr<FloatAttribute>("k_smoothing");

  struct { uint32_t width; uint32_t height; float range_min; float range_max; float hmin; float hmax; float k_smoothing; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile = p_out->tiles[i];

    pc.width       = static_cast<uint32_t>(tile.shape.x);
    pc.height      = static_cast<uint32_t>(tile.shape.y);
    pc.range_min   = range_min;
    pc.range_max   = range_max;
    pc.hmin        = hmin;
    pc.hmax        = hmax;
    pc.k_smoothing = k_smoothing;

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer data_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    data_buf.upload(tile.vector.data(), buf_size);

    std::vector<VulkanBuffer *> buffers = {&data_buf};
    gp.dispatch("saturate", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    data_buf.download(tile.vector.data(), buf_size);
  }

  post_process_heightmap(node, *p_out, p_in);
  return true;
}
#endif

} // namespace hesiod
