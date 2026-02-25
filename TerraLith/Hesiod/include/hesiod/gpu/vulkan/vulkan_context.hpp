/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#ifdef HESIOD_HAS_VULKAN

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace hesiod
{

class VulkanContext
{
public:
  static VulkanContext &instance();

  bool is_ready() const;

  VkDevice         device() const;
  VkPhysicalDevice physical_device() const;
  VkQueue          compute_queue() const;
  uint32_t         compute_queue_family() const;
  VkInstance       vk_instance() const;

  void submit_and_wait(const std::function<void(VkCommandBuffer)> &record_fn);

  uint32_t find_memory_type(uint32_t              type_filter,
                            VkMemoryPropertyFlags properties) const;

  // Global descriptor pool management
  VkDescriptorPool descriptor_pool() const;
  VkDescriptorSet  allocate_descriptor_set(VkDescriptorSetLayout layout);
  void             free_descriptor_set(VkDescriptorSet set);
  void             reset_descriptor_pool();

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;

  ~VulkanContext();

private:
  VulkanContext();

  void init();
  void create_instance();
  void select_physical_device();
  void create_logical_device();
  void create_command_pool();
  void create_descriptor_pool();

  uint32_t find_compute_queue_family() const;

  VkInstance               instance_        = VK_NULL_HANDLE;
  VkPhysicalDevice         physical_device_ = VK_NULL_HANDLE;
  VkDevice                 device_          = VK_NULL_HANDLE;
  VkQueue                  compute_queue_   = VK_NULL_HANDLE;
  uint32_t                 queue_family_    = 0;
  VkCommandPool            command_pool_    = VK_NULL_HANDLE;
  VkDescriptorPool         descriptor_pool_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  bool                     ready_           = false;
};

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
