/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/gradient.hpp"

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

void setup_gradient_angle_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("smoothing_radius", "smoothing_radius", 0.f, 0.f, 0.2f);
  node.add_attr<RangeAttribute>("remap",
                                "remap",
                                std::vector<float>({-1.f, 1.f}),
                                -1.f,
                                1.f,
                                false);

  // attribute(s) order
  node.set_attr_ordered_key({"_TEXT_Post-processing", "smoothing_radius", "remap"});
}

void compute_gradient_angle_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    int ir = (int)(node.get_attr<FloatAttribute>("smoothing_radius") * p_out->shape.x);

    hmap::transform(
        {p_out, p_in},
        [ir](std::vector<hmap::Array *> p_arrays)
        {
          hmap::Array *pa_out = p_arrays[0];
          hmap::Array *pa_in = p_arrays[1];

          if (ir > 0)
            *pa_out = hmap::gradient_angle_circular_smoothing(*pa_in, ir);
          else
            *pa_out = hmap::gradient_angle(*pa_in);
        },
        node.get_config_ref()->hmap_transform_mode_cpu);

    p_out->smooth_overlap_buffers();

    // post-process
    post_process_heightmap(node,
                           *p_out,
                           false, // inverse
                           false, // smooth
                           0,
                           false, // saturate
                           {0.f, 0.f},
                           0.f,
                           node.get_attr_ref<RangeAttribute>("remap")->get_is_active(),
                           node.get_attr<RangeAttribute>("remap"));
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_gradient_angle_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in)
    return false;

  // Only accelerate the basic gradient_angle path (no smoothing radius);
  // circular smoothing with radius > 0 falls back to CPU.
  int ir = (int)(node.get_attr<FloatAttribute>("smoothing_radius") *
                 node.get_value_ref<hmap::Heightmap>("output")->shape.x);
  if (ir > 0)
    return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready())
    return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  struct
  {
    uint32_t width;
    uint32_t height;
  } pc{};

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_in  = p_in->tiles[i];
    auto &tile_out = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile_in.shape.x);
    pc.height = static_cast<uint32_t>(tile_in.shape.y);

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_in.vector.data(), buf_size);

    // slope_aspect shader writes slope to binding 1 and aspect to binding 2;
    // we only need the aspect output here, but must provide all three bindings.
    VulkanBuffer slope_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VulkanBuffer aspect_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VulkanBuffer *> buffers = {&input_buf, &slope_buf, &aspect_buf};
    gp.dispatch("slope_aspect", &pc, sizeof(pc), buffers,
                (pc.width + 15) / 16, (pc.height + 15) / 16);

    aspect_buf.download(tile_out.vector.data(), buf_size);
  }

  p_out->smooth_overlap_buffers();

  // post-process
  post_process_heightmap(node,
                         *p_out,
                         false, // inverse
                         false, // smooth
                         0,
                         false, // saturate
                         {0.f, 0.f},
                         0.f,
                         node.get_attr_ref<RangeAttribute>("remap")->get_is_active(),
                         node.get_attr<RangeAttribute>("remap"));

  return true;
}
#endif

} // namespace hesiod
