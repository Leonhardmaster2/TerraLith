/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#ifdef HESIOD_HAS_VULKAN

#include <cstddef>
#include <cstdint>
#include <memory>

#include <vulkan/vulkan.h>

namespace hesiod
{

class VulkanBuffer;

struct NoiseFbmPushConstants
{
  uint32_t width;
  uint32_t height;
  float    kw_x;
  float    kw_y;
  uint32_t seed;
  int32_t  octaves;
  float    weight;
  float    persistence;
  float    lacunarity;
  int32_t  noise_type;
  float    bbox_x;
  float    bbox_y;
  float    bbox_z;
  float    bbox_w;
};

static_assert(sizeof(NoiseFbmPushConstants) == 56,
              "Push constants must be exactly 56 bytes");

class VulkanNoisePipeline
{
public:
  static VulkanNoisePipeline &instance();

  bool is_ready() const;

  void compute_noise_fbm(float                        *output_data,
                          int                           width,
                          int                           height,
                          const NoiseFbmPushConstants &params);

  VulkanNoisePipeline(const VulkanNoisePipeline &) = delete;
  VulkanNoisePipeline &operator=(const VulkanNoisePipeline &) = delete;

  ~VulkanNoisePipeline();

private:
  VulkanNoisePipeline();

  void init();
  void ensure_capacity(VkDeviceSize required_size);

  // Pipeline objects
  VkDescriptorSetLayout desc_layout_    = VK_NULL_HANDLE;
  VkPipelineLayout      pipeline_layout_ = VK_NULL_HANDLE;
  VkShaderModule        shader_module_   = VK_NULL_HANDLE;
  VkPipeline            pipeline_        = VK_NULL_HANDLE;
  bool                  ready_           = false;

  // Persistent cache
  std::unique_ptr<VulkanBuffer> persistent_staging_buffer_;
  std::unique_ptr<VulkanBuffer> persistent_storage_buffer_;
  VkDescriptorPool              descriptor_pool_ = VK_NULL_HANDLE;
  VkDescriptorSet               descriptor_set_  = VK_NULL_HANDLE;
  size_t                        current_buffer_capacity_ = 0;
};

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
