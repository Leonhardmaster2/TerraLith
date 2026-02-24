/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hesiod/gpu/vulkan/vulkan_generic_pipeline.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/shader_paths.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

// --- File helpers ---

static std::vector<char> read_spirv(const std::string &path)
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

static VkShaderModule create_shader_mod(VkDevice device, const std::vector<char> &code)
{
  VkShaderModuleCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.codeSize = code.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule mod;
  VkResult       r = vkCreateShaderModule(device, &ci, nullptr, &mod);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module, error: " +
                             std::to_string(r));
  return mod;
}

// --- Singleton ---

VulkanGenericPipeline &VulkanGenericPipeline::instance()
{
  static VulkanGenericPipeline inst;
  return inst;
}

VulkanGenericPipeline::VulkanGenericPipeline()
{
  auto &ctx = VulkanContext::instance();
  this->ready_ = ctx.is_ready();
  if (this->ready_)
    Logger::log()->info("VulkanGenericPipeline ready (lazy pipeline creation)");
  else
    Logger::log()->warn("VulkanGenericPipeline: VulkanContext not ready");
}

VulkanGenericPipeline::~VulkanGenericPipeline()
{
  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
    return;

  VkDevice device = ctx.device();

  for (auto &[key, entry] : this->cache_)
  {
    if (entry.pipeline != VK_NULL_HANDLE)
      vkDestroyPipeline(device, entry.pipeline, nullptr);
    if (entry.pipeline_layout != VK_NULL_HANDLE)
      vkDestroyPipelineLayout(device, entry.pipeline_layout, nullptr);
    if (entry.desc_layout != VK_NULL_HANDLE)
      vkDestroyDescriptorSetLayout(device, entry.desc_layout, nullptr);
    if (entry.shader_module != VK_NULL_HANDLE)
      vkDestroyShaderModule(device, entry.shader_module, nullptr);
  }

  Logger::log()->trace("VulkanGenericPipeline destroyed ({} cached pipelines)",
                       this->cache_.size());
}

bool VulkanGenericPipeline::is_ready() const { return this->ready_; }

// --- Cache key ---

std::string VulkanGenericPipeline::make_cache_key(const std::string &shader_name,
                                                   uint32_t           num_bindings,
                                                   uint32_t           push_size) const
{
  return shader_name + ":" + std::to_string(num_bindings) + ":" +
         std::to_string(push_size);
}

// --- Lazy pipeline creation ---

VulkanGenericPipeline::PipelineEntry &
VulkanGenericPipeline::get_or_create(const std::string &shader_name,
                                      uint32_t           num_bindings,
                                      uint32_t           push_size)
{
  std::string key = this->make_cache_key(shader_name, num_bindings, push_size);

  auto it = this->cache_.find(key);
  if (it != this->cache_.end())
    return it->second;

  // Create new pipeline entry
  auto   &ctx = VulkanContext::instance();
  VkDevice device = ctx.device();

  PipelineEntry entry{};
  entry.num_bindings = num_bindings;
  entry.push_size = push_size;

  // Load shader
  std::string spirv_path = VULKAN_SHADER_DIR + "/" + shader_name + ".spv";
  auto        code = read_spirv(spirv_path);
  entry.shader_module = create_shader_mod(device, code);

  // Descriptor set layout: N storage buffers
  std::vector<VkDescriptorSetLayoutBinding> bindings(num_bindings);
  for (uint32_t i = 0; i < num_bindings; ++i)
  {
    bindings[i] = {};
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layout_ci{};
  layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_ci.bindingCount = num_bindings;
  layout_ci.pBindings = bindings.data();

  VkResult r =
      vkCreateDescriptorSetLayout(device, &layout_ci, nullptr, &entry.desc_layout);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set layout for shader: " +
                             shader_name);

  // Pipeline layout
  VkPipelineLayoutCreateInfo pl_ci{};
  pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pl_ci.setLayoutCount = 1;
  pl_ci.pSetLayouts = &entry.desc_layout;

  VkPushConstantRange push_range{};
  if (push_size > 0)
  {
    push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_range.offset = 0;
    push_range.size = push_size;
    pl_ci.pushConstantRangeCount = 1;
    pl_ci.pPushConstantRanges = &push_range;
  }

  r = vkCreatePipelineLayout(device, &pl_ci, nullptr, &entry.pipeline_layout);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout for shader: " +
                             shader_name);

  // Compute pipeline
  VkComputePipelineCreateInfo pipe_ci{};
  pipe_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipe_ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipe_ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipe_ci.stage.module = entry.shader_module;
  pipe_ci.stage.pName = "main";
  pipe_ci.layout = entry.pipeline_layout;

  r = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipe_ci, nullptr,
                               &entry.pipeline);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create compute pipeline for shader: " +
                             shader_name);

  Logger::log()->info("VulkanGenericPipeline: created pipeline for '{}' "
                      "({} bindings, {} bytes push constants)",
                      shader_name,
                      num_bindings,
                      push_size);

  auto [inserted_it, _] = this->cache_.emplace(key, entry);
  return inserted_it->second;
}

// --- Dispatch ---

void VulkanGenericPipeline::dispatch(const std::string                 &shader_name,
                                      const void                        *push_data,
                                      uint32_t                           push_size,
                                      const std::vector<VulkanBuffer *> &buffers,
                                      uint32_t                           group_x,
                                      uint32_t                           group_y,
                                      uint32_t                           group_z)
{
  if (!this->ready_)
    throw std::runtime_error("VulkanGenericPipeline not ready");

  uint32_t num_bindings = static_cast<uint32_t>(buffers.size());

  auto    &entry = this->get_or_create(shader_name, num_bindings, push_size);
  auto    &ctx = VulkanContext::instance();
  VkDevice device = ctx.device();

  // Descriptor pool (per-dispatch, cleaned up after)
  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_size.descriptorCount = num_bindings;

  VkDescriptorPoolCreateInfo pool_ci{};
  pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_ci.maxSets = 1;
  pool_ci.poolSizeCount = 1;
  pool_ci.pPoolSizes = &pool_size;

  VkDescriptorPool desc_pool;
  VkResult r = vkCreateDescriptorPool(device, &pool_ci, nullptr, &desc_pool);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool");

  // Allocate descriptor set
  VkDescriptorSetAllocateInfo desc_alloc{};
  desc_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  desc_alloc.descriptorPool = desc_pool;
  desc_alloc.descriptorSetCount = 1;
  desc_alloc.pSetLayouts = &entry.desc_layout;

  VkDescriptorSet desc_set;
  r = vkAllocateDescriptorSets(device, &desc_alloc, &desc_set);
  if (r != VK_SUCCESS)
  {
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    throw std::runtime_error("Failed to allocate descriptor set");
  }

  // Write descriptor set bindings
  std::vector<VkDescriptorBufferInfo> buf_infos(num_bindings);
  std::vector<VkWriteDescriptorSet>   writes(num_bindings);

  for (uint32_t i = 0; i < num_bindings; ++i)
  {
    buf_infos[i] = {};
    buf_infos[i].buffer = buffers[i]->buffer();
    buf_infos[i].offset = 0;
    buf_infos[i].range = buffers[i]->size();

    writes[i] = {};
    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = desc_set;
    writes[i].dstBinding = i;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].pBufferInfo = &buf_infos[i];
  }

  vkUpdateDescriptorSets(device, num_bindings, writes.data(), 0, nullptr);

  // Record and submit
  ctx.submit_and_wait(
      [&](VkCommandBuffer cmd)
      {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, entry.pipeline);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                entry.pipeline_layout,
                                0,
                                1,
                                &desc_set,
                                0,
                                nullptr);
        if (push_size > 0 && push_data)
          vkCmdPushConstants(cmd,
                             entry.pipeline_layout,
                             VK_SHADER_STAGE_COMPUTE_BIT,
                             0,
                             push_size,
                             push_data);
        vkCmdDispatch(cmd, group_x, group_y, group_z);
      });

  // Cleanup per-dispatch resources
  vkDestroyDescriptorPool(device, desc_pool, nullptr);
}

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
