/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/erosion.hpp"

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

void setup_hydraulic_blur_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<FloatAttribute>("radius", "radius", 0.1f, 0.01f, 0.5f);
  node.add_attr<FloatAttribute>("vmax", "vmax", 0.5f, -1.f, 2.f);
  node.add_attr<FloatAttribute>("k_smoothing", "k_smoothing", 0.1f, 0.f, 1.f);
  node.add_attr<RangeAttribute>("remap", "remap");

  // attribute(s) order
  node.set_attr_ordered_key({"radius", "vmax", "k_smoothing", "_SEPARATOR_", "remap"});
}

void compute_hydraulic_blur_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    // copy the input heightmap
    *p_out = *p_in;

    hmap::transform(*p_out,
                    [&node](hmap::Array &out)
                    {
                      hmap::hydraulic_blur(out,
                                           node.get_attr<FloatAttribute>("radius"),
                                           node.get_attr<FloatAttribute>("vmax"),
                                           node.get_attr<FloatAttribute>("k_smoothing"));
                    });

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
bool compute_hydraulic_blur_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  float radius      = node.get_attr<FloatAttribute>("radius");
  float vmax        = node.get_attr<FloatAttribute>("vmax");
  float k_smoothing = node.get_attr<FloatAttribute>("k_smoothing");

  // Cap GPU path at ir <= 64 to keep the 2D kernel practical;
  // larger radii fall back to CPU.
  int ir_check = std::max(1, (int)(radius * p_out->shape.x));
  if (ir_check > 64) return false;

  // Copy input to output first (matching CPU path)
  *p_out = *p_in;

  struct
  {
    uint32_t width;
    uint32_t height;
    float    vmax;
    int32_t  ir;
    float    k_smoothing;
  } pc{};

  pc.vmax        = vmax;
  pc.k_smoothing = k_smoothing;

  for (size_t i = 0; i < p_out->get_ntiles(); ++i)
  {
    auto &tile_out = p_out->tiles[i];

    pc.width  = static_cast<uint32_t>(tile_out.shape.x);
    pc.height = static_cast<uint32_t>(tile_out.shape.y);
    pc.ir     = std::max(1, (int)(radius * tile_out.shape.x));

    VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

    VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    input_buf.upload(tile_out.vector.data(), buf_size);

    VulkanBuffer output_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VulkanBuffer *> buffers = {&input_buf, &output_buf};
    gp.dispatch("hydraulic_blur", &pc, sizeof(pc), buffers,
                (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
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
