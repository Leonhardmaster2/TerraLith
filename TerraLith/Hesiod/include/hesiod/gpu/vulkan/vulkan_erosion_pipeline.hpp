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

struct ErosionPushConstants
{
  uint32_t width;
  uint32_t height;
  float    c_erosion;
  float    talus_ref;
  float    clipping_ratio;
  uint32_t iteration;
  uint32_t pass_type; // 0 = flow accumulation, 1 = erosion apply
};

static_assert(sizeof(ErosionPushConstants) == 28,
              "ErosionPushConstants must be exactly 28 bytes");

struct ErosionPerformanceMetrics
{
  uint32_t iteration_count;
  double   total_gpu_dispatch_ms;
  double   per_iteration_avg_ms;
  double   memory_barrier_stall_ms;
  double   cpu_baseline_ms;       // CPU/OpenCL single-pass time
  double   vulkan_total_ms;       // Total Vulkan time including setup
  double   speedup_factor;        // cpu_baseline / vulkan_total
};

class VulkanErosionPipeline
{
public:
  static VulkanErosionPipeline &instance();

  bool is_ready() const;

  // Run the full hydraulic erosion simulation on the GPU.
  //   heightmap_data   : in/out heightmap float array (width * height)
  //   erosion_data     : out erosion map (may be nullptr)
  //   mask_data        : optional mask (nullptr = all 1s)
  //   width, height    : dimensions
  //   c_erosion        : erosion coefficient
  //   talus_ref        : talus reference slope
  //   clipping_ratio   : flow accumulation clipping ratio
  //   num_iterations   : number of flow accumulation relaxation iterations
  //   metrics          : output performance metrics
  ErosionPerformanceMetrics compute_erosion(
      float   *heightmap_data,
      float   *erosion_data,
      const float *mask_data,
      uint32_t width,
      uint32_t height,
      float    c_erosion,
      float    talus_ref,
      float    clipping_ratio,
      uint32_t num_iterations);

  VulkanErosionPipeline(const VulkanErosionPipeline &) = delete;
  VulkanErosionPipeline &operator=(const VulkanErosionPipeline &) = delete;

  ~VulkanErosionPipeline();

private:
  VulkanErosionPipeline();

  void init();
  void ensure_capacity(uint32_t width, uint32_t height);

  // Pipeline objects
  VkDescriptorSetLayout desc_layout_     = VK_NULL_HANDLE;
  VkPipelineLayout      pipeline_layout_ = VK_NULL_HANDLE;
  VkShaderModule        shader_module_   = VK_NULL_HANDLE;
  VkPipeline            pipeline_        = VK_NULL_HANDLE;
  bool                  ready_           = false;

  // Global descriptor pool (persistent)
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkDescriptorSet  descriptor_set_  = VK_NULL_HANDLE;

  // Persistent GPU buffers
  std::unique_ptr<VulkanBuffer> heightmap_buf_;
  std::unique_ptr<VulkanBuffer> flow_acc_buf_;
  std::unique_ptr<VulkanBuffer> erosion_buf_;
  std::unique_ptr<VulkanBuffer> mask_buf_;

  size_t current_capacity_ = 0; // in floats
};

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
