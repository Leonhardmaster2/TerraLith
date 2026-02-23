/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#ifdef HESIOD_HAS_VULKAN

#include <cstddef>
#include <vector>

#include <vulkan/vulkan.h>

namespace hesiod
{

class VulkanBuffer
{
public:
  VulkanBuffer(VkDeviceSize          size_bytes,
               VkBufferUsageFlags    usage,
               VkMemoryPropertyFlags memory_properties);

  ~VulkanBuffer();

  VulkanBuffer(VulkanBuffer &&other) noexcept;
  VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;

  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer &operator=(const VulkanBuffer &) = delete;

  void upload(const void *data, VkDeviceSize size);
  void download(void *data, VkDeviceSize size) const;

  void               upload_floats(const std::vector<float> &data);
  std::vector<float> download_floats(size_t count) const;

  VkBuffer     buffer() const { return buffer_; }
  VkDeviceSize size() const { return size_; }

private:
  void cleanup();

  VkBuffer       buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkDeviceSize   size_   = 0;
};

VulkanBuffer create_storage_buffer(VkDeviceSize size_bytes);
VulkanBuffer create_staging_buffer(VkDeviceSize size_bytes);

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
