/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

VulkanBuffer::VulkanBuffer(VkDeviceSize          size_bytes,
                           VkBufferUsageFlags    usage,
                           VkMemoryPropertyFlags memory_properties)
    : size_(size_bytes)
{
  auto &ctx = VulkanContext::instance();

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size_bytes;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(ctx.device(), &buffer_info, nullptr, &this->buffer_);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan buffer, error: " +
                             std::to_string(result));

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(ctx.device(), this->buffer_, &mem_reqs);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = ctx.find_memory_type(mem_reqs.memoryTypeBits,
                                                     memory_properties);

  result = vkAllocateMemory(ctx.device(), &alloc_info, nullptr, &this->memory_);
  if (result != VK_SUCCESS)
  {
    vkDestroyBuffer(ctx.device(), this->buffer_, nullptr);
    this->buffer_ = VK_NULL_HANDLE;
    throw std::runtime_error("Failed to allocate Vulkan buffer memory, error: " +
                             std::to_string(result));
  }

  vkBindBufferMemory(ctx.device(), this->buffer_, this->memory_, 0);
}

VulkanBuffer::~VulkanBuffer() { this->cleanup(); }

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
    : buffer_(other.buffer_), memory_(other.memory_), size_(other.size_)
{
  other.buffer_ = VK_NULL_HANDLE;
  other.memory_ = VK_NULL_HANDLE;
  other.size_ = 0;
}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept
{
  if (this != &other)
  {
    this->cleanup();
    this->buffer_ = other.buffer_;
    this->memory_ = other.memory_;
    this->size_ = other.size_;
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_ = 0;
  }
  return *this;
}

void VulkanBuffer::cleanup()
{
  auto &ctx = VulkanContext::instance();

  if (this->buffer_ != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(ctx.device(), this->buffer_, nullptr);
    this->buffer_ = VK_NULL_HANDLE;
  }
  if (this->memory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(ctx.device(), this->memory_, nullptr);
    this->memory_ = VK_NULL_HANDLE;
  }
  this->size_ = 0;
}

void VulkanBuffer::upload(const void *data, VkDeviceSize size)
{
  auto &ctx = VulkanContext::instance();

  void *mapped = nullptr;
  VkResult result = vkMapMemory(ctx.device(), this->memory_, 0, size, 0, &mapped);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to map Vulkan buffer memory for upload");

  std::memcpy(mapped, data, static_cast<size_t>(size));
  vkUnmapMemory(ctx.device(), this->memory_);
}

void VulkanBuffer::download(void *data, VkDeviceSize size) const
{
  auto &ctx = VulkanContext::instance();

  void *mapped = nullptr;
  VkResult result = vkMapMemory(ctx.device(), this->memory_, 0, size, 0, &mapped);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to map Vulkan buffer memory for download");

  std::memcpy(data, mapped, static_cast<size_t>(size));
  vkUnmapMemory(ctx.device(), this->memory_);
}

void VulkanBuffer::upload_floats(const std::vector<float> &data)
{
  this->upload(data.data(),
               static_cast<VkDeviceSize>(data.size() * sizeof(float)));
}

std::vector<float> VulkanBuffer::download_floats(size_t count) const
{
  std::vector<float> result(count);
  this->download(result.data(), static_cast<VkDeviceSize>(count * sizeof(float)));
  return result;
}

// --- Factory helpers ---

VulkanBuffer create_storage_buffer(VkDeviceSize size_bytes)
{
  return VulkanBuffer(size_bytes,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

VulkanBuffer create_staging_buffer(VkDeviceSize size_bytes)
{
  return VulkanBuffer(size_bytes,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
