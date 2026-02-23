/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/vulkan_noise_pipeline.hpp"
#include "hesiod/gpu/vulkan/shader_paths.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

static std::vector<char> read_spirv_file(const std::string &path)
{
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open SPIR-V file: " + path);

  size_t            file_size = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(file_size);
  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(file_size));
  return buffer;
}

static VkShaderModule create_shader_module(VkDevice                   device,
                                           const std::vector<char> &code)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule module;
  VkResult       result = vkCreateShaderModule(device, &create_info, nullptr, &module);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create noise shader module, error: " +
                             std::to_string(result));

  return module;
}

// --- Singleton ---

VulkanNoisePipeline &VulkanNoisePipeline::instance()
{
  static VulkanNoisePipeline inst;
  return inst;
}

VulkanNoisePipeline::VulkanNoisePipeline() { this->init(); }

VulkanNoisePipeline::~VulkanNoisePipeline()
{
  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
    return;

  VkDevice device = ctx.device();

  if (this->pipeline_ != VK_NULL_HANDLE)
    vkDestroyPipeline(device, this->pipeline_, nullptr);
  if (this->pipeline_layout_ != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, this->pipeline_layout_, nullptr);
  if (this->desc_layout_ != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, this->desc_layout_, nullptr);
  if (this->shader_module_ != VK_NULL_HANDLE)
    vkDestroyShaderModule(device, this->shader_module_, nullptr);

  Logger::log()->trace("VulkanNoisePipeline destroyed");
}

void VulkanNoisePipeline::init()
{
  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
  {
    Logger::log()->warn("VulkanNoisePipeline::init: VulkanContext not ready, skipping");
    return;
  }

  try
  {
    VkDevice device = ctx.device();

    // --- Load shader ---
    std::string spirv_path = VULKAN_SHADER_DIR + "/noise_fbm.spv";
    auto        shader_code = read_spirv_file(spirv_path);
    this->shader_module_ = create_shader_module(device, shader_code);

    // --- Descriptor set layout: 1 SSBO (output) ---
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    VkResult result =
        vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &this->desc_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create descriptor set layout");

    // --- Pipeline layout: push constants + descriptor set ---
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(NoiseFbmPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &this->desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_range;

    result = vkCreatePipelineLayout(device,
                                    &pipeline_layout_info,
                                    nullptr,
                                    &this->pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create pipeline layout");

    // --- Compute pipeline ---
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = this->shader_module_;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = this->pipeline_layout_;

    result = vkCreateComputePipelines(device,
                                      VK_NULL_HANDLE,
                                      1,
                                      &pipeline_info,
                                      nullptr,
                                      &this->pipeline_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute pipeline");

    this->ready_ = true;
    Logger::log()->info("VulkanNoisePipeline initialized successfully");
  }
  catch (const std::exception &e)
  {
    Logger::log()->error("VulkanNoisePipeline initialization failed: {}", e.what());
    this->ready_ = false;
  }
}

bool VulkanNoisePipeline::is_ready() const { return this->ready_; }

void VulkanNoisePipeline::compute_noise_fbm(float                        *output_data,
                                             int                           width,
                                             int                           height,
                                             const NoiseFbmPushConstants &params)
{
  if (!this->ready_)
    throw std::runtime_error("VulkanNoisePipeline not ready");

  auto    &ctx = VulkanContext::instance();
  VkDevice device = ctx.device();

  VkDeviceSize buffer_size =
      static_cast<VkDeviceSize>(width) * height * sizeof(float);

  // Use host-visible + storage buffer for simplicity (avoids staging copy)
  VulkanBuffer output_buf(buffer_size,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // --- Descriptor pool and set (per-dispatch) ---
  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;

  VkDescriptorPool desc_pool;
  VkResult result = vkCreateDescriptorPool(device, &pool_info, nullptr, &desc_pool);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool");

  VkDescriptorSetAllocateInfo desc_alloc{};
  desc_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  desc_alloc.descriptorPool = desc_pool;
  desc_alloc.descriptorSetCount = 1;
  desc_alloc.pSetLayouts = &this->desc_layout_;

  VkDescriptorSet desc_set;
  result = vkAllocateDescriptorSets(device, &desc_alloc, &desc_set);
  if (result != VK_SUCCESS)
  {
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    throw std::runtime_error("Failed to allocate descriptor set");
  }

  // Write descriptor
  VkDescriptorBufferInfo buf_info{};
  buf_info.buffer = output_buf.buffer();
  buf_info.offset = 0;
  buf_info.range = buffer_size;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = desc_set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.pBufferInfo = &buf_info;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  // --- Dispatch compute ---
  uint32_t group_x = (static_cast<uint32_t>(width) + 15) / 16;
  uint32_t group_y = (static_cast<uint32_t>(height) + 15) / 16;

  ctx.submit_and_wait(
      [&](VkCommandBuffer cmd)
      {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline_);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                this->pipeline_layout_,
                                0,
                                1,
                                &desc_set,
                                0,
                                nullptr);
        vkCmdPushConstants(cmd,
                           this->pipeline_layout_,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           0,
                           sizeof(NoiseFbmPushConstants),
                           &params);
        vkCmdDispatch(cmd, group_x, group_y, 1);
      });

  // --- Download results ---
  output_buf.download(output_data, buffer_size);

  // --- Cleanup per-dispatch resources ---
  vkDestroyDescriptorPool(device, desc_pool, nullptr);
}

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
