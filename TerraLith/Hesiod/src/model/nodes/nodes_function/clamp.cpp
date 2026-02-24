/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

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

void setup_clamp_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<RangeAttribute>("clamp", "clamp");
  node.add_attr<BoolAttribute>("smooth_min", "smooth_min", false);
  node.add_attr<FloatAttribute>("k_min", "k_min", 0.05f, 0.01f, 1.f);
  node.add_attr<BoolAttribute>("smooth_max", "smooth_max", false);
  node.add_attr<FloatAttribute>("k_max", "k_max", 0.05f, 0.01f, 1.f);
  node.add_attr<BoolAttribute>("remap", "remap", false);

  // link histogram for RangeAttribute
  setup_histogram_for_range_attribute(node, "clamp", "input");

  // attribute(s) order
  node.set_attr_ordered_key(
      {"clamp", "smooth_min", "k_min", "smooth_max", "k_max", "remap"});

  setup_post_process_heightmap_attributes(node, true);
}

void compute_clamp_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // copy the input heightmap
    *p_out = *p_in;

    // retrieve parameters
    hmap::Vec2<float> crange = node.get_attr<RangeAttribute>("clamp");
    bool              smooth_min = node.get_attr<BoolAttribute>("smooth_min");
    bool              smooth_max = node.get_attr<BoolAttribute>("smooth_max");
    float             k_min = node.get_attr<FloatAttribute>("k_min");
    float             k_max = node.get_attr<FloatAttribute>("k_max");

    // compute
    if (!smooth_min && !smooth_max)
    {
      hmap::transform(
          {p_out},
          [&crange](std::vector<hmap::Array *> p_arrays)
          {
            hmap::Array *pa_out = p_arrays[0];
            hmap::clamp(*pa_out, crange.x, crange.y);
          },
          node.get_config_ref()->hmap_transform_mode_cpu);
    }
    else
    {
      if (smooth_min)
        hmap::transform(
            {p_out},
            [&crange, &k_min](std::vector<hmap::Array *> p_arrays)
            {
              hmap::Array *pa_out = p_arrays[0];
              hmap::clamp_min_smooth(*pa_out, crange.x, k_min);
            },
            node.get_config_ref()->hmap_transform_mode_cpu);
      else
        hmap::transform(
            {p_out},
            [&crange](std::vector<hmap::Array *> p_arrays)
            {
              hmap::Array *pa_out = p_arrays[0];
              hmap::clamp_min(*pa_out, crange.x);
            },
            node.get_config_ref()->hmap_transform_mode_cpu);

      if (smooth_max)
        hmap::transform(
            {p_out},
            [&crange, &k_max](std::vector<hmap::Array *> p_arrays)
            {
              hmap::Array *pa_out = p_arrays[0];
              hmap::clamp_max_smooth(*pa_out, crange.y, k_max);
            },
            node.get_config_ref()->hmap_transform_mode_cpu);
      else
        hmap::transform(
            {p_out},
            [&crange](std::vector<hmap::Array *> p_arrays)
            {
              hmap::Array *pa_out = p_arrays[0];
              hmap::clamp_max(*pa_out, crange.y);
            },
            node.get_config_ref()->hmap_transform_mode_cpu);
    }

    if (node.get_attr<BoolAttribute>("remap"))
      p_out->remap();

    // post-process
    post_process_heightmap(node, *p_out, p_in);
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_clamp_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  // Only accelerate simple (non-smooth) clamp on GPU
  if (node.get_attr<BoolAttribute>("smooth_min") || node.get_attr<BoolAttribute>("smooth_max"))
    return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap  *p_out = node.get_value_ref<hmap::Heightmap>("output");
  hmap::Vec2<float> crange = node.get_attr<RangeAttribute>("clamp");

  struct { uint32_t width; uint32_t height; float clamp_min; float clamp_max; } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_in  = p_in->tiles[i];
    auto &tile_out = p_out->tiles[i];

    pc.width     = static_cast<uint32_t>(tile_in.shape.x);
    pc.height    = static_cast<uint32_t>(tile_in.shape.y);
    pc.clamp_min = crange.x;
    pc.clamp_max = crange.y;

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_in.vector.data(), buf_size);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    std::vector<VulkanBuffer *> buffers = {&input_buf, &output_buf};
    gp.dispatch("clamp", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  if (node.get_attr<BoolAttribute>("remap"))
    p_out->remap();

  post_process_heightmap(node, *p_out, p_in);
  return true;
}
#endif

} // namespace hesiod
