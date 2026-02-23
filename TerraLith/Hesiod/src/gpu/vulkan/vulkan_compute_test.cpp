/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_compute_test.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/shader_paths.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

static std::vector<char> read_spirv_file(const std::string &path)
{
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open SPIR-V file: " + path);

  size_t file_size = static_cast<size_t>(file.tellg());
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
  VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &module);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module");

  return module;
}

bool VulkanComputeTest::run_add_test(size_t num_elements)
{
  Logger::log()->info("VulkanComputeTest::run_add_test: {} elements", num_elements);

  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
  {
    Logger::log()->error("VulkanComputeTest: Vulkan context not ready");
    return false;
  }

  VkDevice device = ctx.device();

  try
  {
    VkDeviceSize buffer_size =
        static_cast<VkDeviceSize>(num_elements * sizeof(float));

    // --- Prepare test data ---
    std::vector<float> data_a(num_elements);
    std::vector<float> data_b(num_elements);
    for (size_t i = 0; i < num_elements; ++i)
    {
      data_a[i] = static_cast<float>(i);
      data_b[i] = static_cast<float>(i) * 2.0f;
    }

    // --- Create buffers ---
    // Use host-visible + storage for simplicity in this proof-of-concept
    VulkanBuffer buf_a(buffer_size,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VulkanBuffer buf_b(buffer_size,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VulkanBuffer buf_c(buffer_size,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    buf_a.upload_floats(data_a);
    buf_b.upload_floats(data_b);

    // --- Load shader ---
    std::string spirv_path = VULKAN_SHADER_DIR + "/vector_add.spv";
    auto shader_code = read_spirv_file(spirv_path);
    VkShaderModule shader_module = create_shader_module(device, shader_code);

    // --- Descriptor set layout ---
    VkDescriptorSetLayoutBinding bindings[3] = {};
    for (int i = 0; i < 3; ++i)
    {
      bindings[i].binding = static_cast<uint32_t>(i);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      bindings[i].descriptorCount = 1;
      bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 3;
    layout_info.pBindings = bindings;

    VkDescriptorSetLayout desc_layout;
    vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &desc_layout);

    // --- Push constant range ---
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(uint32_t);

    // --- Pipeline layout ---
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &desc_layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_range;

    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout);

    // --- Compute pipeline ---
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = pipeline_layout;

    VkPipeline pipeline;
    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                             &pipeline);

    // --- Descriptor pool and set ---
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 3;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VkDescriptorPool desc_pool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &desc_pool);

    VkDescriptorSetAllocateInfo desc_alloc{};
    desc_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc.descriptorPool = desc_pool;
    desc_alloc.descriptorSetCount = 1;
    desc_alloc.pSetLayouts = &desc_layout;

    VkDescriptorSet desc_set;
    vkAllocateDescriptorSets(device, &desc_alloc, &desc_set);

    // Write descriptor set
    VkDescriptorBufferInfo buffer_infos[3] = {};
    buffer_infos[0] = {buf_a.buffer(), 0, buffer_size};
    buffer_infos[1] = {buf_b.buffer(), 0, buffer_size};
    buffer_infos[2] = {buf_c.buffer(), 0, buffer_size};

    VkWriteDescriptorSet writes[3] = {};
    for (int i = 0; i < 3; ++i)
    {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = desc_set;
      writes[i].dstBinding = static_cast<uint32_t>(i);
      writes[i].descriptorCount = 1;
      writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      writes[i].pBufferInfo = &buffer_infos[i];
    }
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    // --- Dispatch ---
    uint32_t count = static_cast<uint32_t>(num_elements);
    uint32_t group_count = (count + 255) / 256;

    ctx.submit_and_wait(
        [&](VkCommandBuffer cmd)
        {
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
          vkCmdBindDescriptorSets(cmd,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  pipeline_layout,
                                  0,
                                  1,
                                  &desc_set,
                                  0,
                                  nullptr);
          vkCmdPushConstants(cmd,
                             pipeline_layout,
                             VK_SHADER_STAGE_COMPUTE_BIT,
                             0,
                             sizeof(uint32_t),
                             &count);
          vkCmdDispatch(cmd, group_count, 1, 1);
        });

    // --- Read back and verify ---
    auto result_data = buf_c.download_floats(num_elements);

    int errors = 0;
    for (size_t i = 0; i < num_elements; ++i)
    {
      float expected = data_a[i] + data_b[i];
      if (std::fabs(result_data[i] - expected) > 1e-5f)
      {
        if (errors < 10)
          Logger::log()->error("  Mismatch at [{}]: got {} expected {}",
                               i,
                               result_data[i],
                               expected);
        ++errors;
      }
    }

    // --- Cleanup Vulkan objects ---
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    vkDestroyDescriptorSetLayout(device, desc_layout, nullptr);
    vkDestroyShaderModule(device, shader_module, nullptr);

    if (errors == 0)
    {
      Logger::log()->info("VulkanComputeTest::run_add_test PASSED ({} elements)",
                          num_elements);
      return true;
    }
    else
    {
      Logger::log()->error(
          "VulkanComputeTest::run_add_test FAILED ({} errors out of {} elements)",
          errors,
          num_elements);
      return false;
    }
  }
  catch (const std::exception &e)
  {
    Logger::log()->error("VulkanComputeTest::run_add_test exception: {}", e.what());
    return false;
  }
}

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
