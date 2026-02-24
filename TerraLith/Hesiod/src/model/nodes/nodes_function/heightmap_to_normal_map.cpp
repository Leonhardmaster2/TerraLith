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

void setup_heightmap_to_normal_map_node(BaseNode &node)
{
  Logger::log()->trace("setup node {}", node.get_label());

  // port(s)
  node.add_port<hmap::Heightmap>(gnode::PortType::IN, "input");
  node.add_port<hmap::HeightmapRGBA>(gnode::PortType::OUT, "normal map", CONFIG(node));

  // attribute(s)

  // attribute(s) order
}

void compute_heightmap_to_normal_map_node(BaseNode &node)
{
  Logger::log()->trace("computing node [{}]/[{}]", node.get_label(), node.get_id());

  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");

  if (p_in)
  {
    hmap::HeightmapRGBA *p_nmap = node.get_value_ref<hmap::HeightmapRGBA>("normal map");

    hmap::Array  array = p_in->to_array();
    hmap::Tensor tn = hmap::normal_map(array);

    hmap::Array nx = tn.get_slice(0);
    hmap::Array ny = tn.get_slice(1);
    hmap::Array nz = tn.get_slice(2);

    *p_nmap = hmap::HeightmapRGBA(p_in->shape,
                                  p_in->tiling,
                                  p_in->overlap,
                                  nx,
                                  ny,
                                  nz,
                                  hmap::Array(p_in->shape, 1.f));
  }
}

#ifdef HESIOD_HAS_VULKAN
bool compute_heightmap_to_normal_map_node_vulkan(BaseNode &node)
{
  hmap::Heightmap *p_in = node.get_value_ref<hmap::Heightmap>("input");
  if (!p_in) return false;

  auto &gp = VulkanGenericPipeline::instance();
  if (!gp.is_ready()) return false;

  hmap::HeightmapRGBA *p_nmap = node.get_value_ref<hmap::HeightmapRGBA>("normal map");

  // Convert full heightmap to a single array (matching CPU path)
  hmap::Array array = p_in->to_array();

  struct { uint32_t width; uint32_t height; } pc{};
  pc.width  = static_cast<uint32_t>(array.shape.x);
  pc.height = static_cast<uint32_t>(array.shape.y);

  VkDeviceSize buf_size = static_cast<VkDeviceSize>(pc.width) * pc.height * sizeof(float);

  VulkanBuffer input_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  input_buf.upload(array.vector.data(), buf_size);

  VulkanBuffer out_r_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VulkanBuffer out_g_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VulkanBuffer out_b_buf(buf_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  std::vector<VulkanBuffer *> buffers = {&input_buf, &out_r_buf, &out_g_buf, &out_b_buf};
  gp.dispatch("normal_map", &pc, sizeof(pc), buffers,
              (pc.width + 15) / 16, (pc.height + 15) / 16);

  hmap::Array nx(array.shape);
  hmap::Array ny(array.shape);
  hmap::Array nz(array.shape);

  out_r_buf.download(nx.vector.data(), buf_size);
  out_g_buf.download(ny.vector.data(), buf_size);
  out_b_buf.download(nz.vector.data(), buf_size);

  *p_nmap = hmap::HeightmapRGBA(p_in->shape,
                                p_in->tiling,
                                p_in->overlap,
                                nx,
                                ny,
                                nz,
                                hmap::Array(p_in->shape, 1.f));

  return true;
}
#endif

} // namespace hesiod
