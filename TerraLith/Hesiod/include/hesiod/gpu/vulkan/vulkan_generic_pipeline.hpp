/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#ifdef HESIOD_HAS_VULKAN

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"

namespace hesiod
{

class VulkanGenericPipeline
{
public:
  static VulkanGenericPipeline &instance();

  bool is_ready() const;

  // Dispatch a compute shader.
  //   shader_name  : base name of .spv file (e.g. "abs" loads abs.spv)
  //   push_data    : pointer to push constant struct (may be nullptr if push_size==0)
  //   push_size    : size in bytes of push constants (0 if none)
  //   buffers      : ordered SSBO pointers (binding 0, 1, 2, ...)
  //   group_x/y/z  : workgroup dispatch counts
  void dispatch(const std::string              &shader_name,
                const void                     *push_data,
                uint32_t                        push_size,
                const std::vector<VulkanBuffer *> &buffers,
                uint32_t                        group_x,
                uint32_t                        group_y,
                uint32_t                        group_z = 1);

  VulkanGenericPipeline(const VulkanGenericPipeline &) = delete;
  VulkanGenericPipeline &operator=(const VulkanGenericPipeline &) = delete;

  ~VulkanGenericPipeline();

private:
  VulkanGenericPipeline();

  struct PipelineEntry
  {
    VkShaderModule        shader_module   = VK_NULL_HANDLE;
    VkDescriptorSetLayout desc_layout     = VK_NULL_HANDLE;
    VkPipelineLayout      pipeline_layout = VK_NULL_HANDLE;
    VkPipeline            pipeline        = VK_NULL_HANDLE;
    uint32_t              num_bindings    = 0;
    uint32_t              push_size       = 0;
  };

  // Cache key: "shader_name:num_bindings:push_size"
  std::string make_cache_key(const std::string &shader_name,
                             uint32_t           num_bindings,
                             uint32_t           push_size) const;

  PipelineEntry &get_or_create(const std::string &shader_name,
                               uint32_t           num_bindings,
                               uint32_t           push_size);

  std::unordered_map<std::string, PipelineEntry> cache_;
  bool                                           ready_ = false;
};

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
