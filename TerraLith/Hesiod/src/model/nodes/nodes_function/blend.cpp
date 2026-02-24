/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/blending.hpp"
#include "highmap/range.hpp"

#include "attributes.hpp"

#include "hesiod/app/enum_mappings.hpp"
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

void setup_blend_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input 1");
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input 2");
  node.add_port<hmap::Heightmap>(gnode::PortType::OUT, "output", CONFIG(node));

  // attribute(s)
  node.add_attr<EnumAttribute>("blending_method",
                               "Method:",
                               enum_mappings.blending_method_map,
                               "minimum_smooth");
  node.add_attr<FloatAttribute>("k", "k", 0.1f, 0.01f, 1.f);
  node.add_attr<FloatAttribute>("radius", "radius", 0.05f, 0.f, 0.2f);
  node.add_attr<FloatAttribute>("input1_weight", "input1_weight", 1.f, 0.f, 1.f);
  node.add_attr<FloatAttribute>("input2_weight", "input2_weight", 1.f, 0.f, 1.f);
  node.add_attr<BoolAttribute>("swap_inputs", "swap_inputs", false);
  node.add_attr<BoolAttribute>("inverse", "inverse", false);
  node.add_attr<RangeAttribute>("remap", "remap");

  // attribute(s) order
  node.set_attr_ordered_key({"_GROUPBOX_BEGIN_Main Parameters",
                             "blending_method",
                             "k",
                             "radius",
                             "_GROUPBOX_END_",
                             //
                             "_GROUPBOX_BEGIN_Inputs",
                             "input1_weight",
                             "input2_weight",
                             "swap_inputs",
                             "_GROUPBOX_END_",
                             //
                             "_GROUPBOX_BEGIN_Post-processing",
                             "inverse",
                             "remap",
                             "_GROUPBOX_END_"});
}

void compute_blend_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in1 = node.get_value_ref<hmap::Heightmap>("input 1");
  hmap::Heightmap *p_in2 = node.get_value_ref<hmap::Heightmap>("input 2");

  if (p_in1 && p_in2)
  {
    // adjust inputs
    if (node.get_attr<BoolAttribute>("swap_inputs"))
      std::swap(p_in1, p_in2);

    // compute output
    hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

    float k = node.get_attr<FloatAttribute>("k");
    int ir = std::max(1, (int)(node.get_attr<FloatAttribute>("radius") * p_out->shape.x));
    int method = node.get_attr<EnumAttribute>("blending_method");

    blend_heightmaps(*p_out,
                     *p_in1,
                     *p_in2,
                     static_cast<BlendingMethod>(method),
                     k,
                     ir,
                     node.get_attr<FloatAttribute>("input1_weight"),
                     node.get_attr<FloatAttribute>("input2_weight"));

    // post-process
    post_process_heightmap(node,
                           *p_out,
                           node.get_attr<BoolAttribute>("inverse"),
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
bool compute_blend_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in1 = node.get_value_ref<hmap::Heightmap>("input 1");
  hmap::Heightmap *p_in2 = node.get_value_ref<hmap::Heightmap>("input 2");
  if (!p_in1 || !p_in2) return false;

  int method = node.get_attr<EnumAttribute>("blending_method");

  // GPU-unsupported methods (require spatial neighborhood or complex ops):
  // EXCLUSION_BLEND=1, GRADIENTS=2, OVERLAY=10, SOFT=12
  if (method == BlendingMethod::EXCLUSION_BLEND ||
      method == BlendingMethod::GRADIENTS ||
      method == BlendingMethod::OVERLAY ||
      method == BlendingMethod::SOFT)
    return false; // fall back to CPU

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  // adjust inputs
  if (node.get_attr<BoolAttribute>("swap_inputs"))
    std::swap(p_in1, p_in2);

  hmap::Heightmap *p_out = node.get_value_ref<hmap::Heightmap>("output");

  float k = node.get_attr<FloatAttribute>("k");
  float w1 = node.get_attr<FloatAttribute>("input1_weight");
  float w2 = node.get_attr<FloatAttribute>("input2_weight");

  struct
  {
    uint32_t width;
    uint32_t height;
    int      method;
    float    k;
    float    weight1;
    float    weight2;
  } pc{};

  pc.method  = method;
  pc.k       = k;
  pc.weight1 = w1;
  pc.weight2 = w2;

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
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VulkanBuffer *> buffers = {&input1_buf, &input2_buf, &output_buf};
    gp.dispatch("combiner_blend", &pc, sizeof(pc), buffers, (pc.width + 15) / 16, (pc.height + 15) / 16);

    output_buf.download(tile_out.vector.data(), buf_size);
  }

  // post-process (on CPU — lightweight)
  post_process_heightmap(node,
                         *p_out,
                         node.get_attr<BoolAttribute>("inverse"),
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
