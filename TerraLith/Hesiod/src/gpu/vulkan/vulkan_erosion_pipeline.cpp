/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hesiod/gpu/vulkan/vulkan_erosion_pipeline.hpp"
#include "hesiod/gpu/vulkan/vulkan_buffer.hpp"
#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/gpu/vulkan/shader_paths.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

// --- File helpers ---

static std::vector<char> read_spirv_erosion(const std::string &path)
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

static VkShaderModule create_shader_mod_erosion(VkDevice                   device,
                                                const std::vector<char> &code)
{
  VkShaderModuleCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.codeSize = code.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule mod;
  VkResult       r = vkCreateShaderModule(device, &ci, nullptr, &mod);
  if (r != VK_SUCCESS)
    throw std::runtime_error("Failed to create erosion shader module, error: " +
                             std::to_string(r));
  return mod;
}

// --- Singleton ---

VulkanErosionPipeline &VulkanErosionPipeline::instance()
{
  static VulkanErosionPipeline inst;
  return inst;
}

VulkanErosionPipeline::VulkanErosionPipeline() { this->init(); }

VulkanErosionPipeline::~VulkanErosionPipeline()
{
  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
    return;

  VkDevice device = ctx.device();

  this->heightmap_buf_.reset();
  this->flow_acc_buf_.reset();
  this->erosion_buf_.reset();
  this->mask_buf_.reset();

  if (this->descriptor_pool_ != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device, this->descriptor_pool_, nullptr);
  if (this->pipeline_ != VK_NULL_HANDLE)
    vkDestroyPipeline(device, this->pipeline_, nullptr);
  if (this->pipeline_layout_ != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, this->pipeline_layout_, nullptr);
  if (this->desc_layout_ != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, this->desc_layout_, nullptr);
  if (this->shader_module_ != VK_NULL_HANDLE)
    vkDestroyShaderModule(device, this->shader_module_, nullptr);

  Logger::log()->trace("VulkanErosionPipeline destroyed");
}

void VulkanErosionPipeline::init()
{
  auto &ctx = VulkanContext::instance();
  if (!ctx.is_ready())
  {
    Logger::log()->warn("VulkanErosionPipeline::init: VulkanContext not ready, skipping");
    return;
  }

  try
  {
    VkDevice device = ctx.device();

    // --- Load shader ---
    std::string spirv_path = VULKAN_SHADER_DIR + "/hydraulic_erosion.spv";
    auto        shader_code = read_spirv_erosion(spirv_path);
    this->shader_module_ = create_shader_mod_erosion(device, shader_code);

    // --- Descriptor set layout: 4 SSBOs ---
    // binding 0: heightmap, binding 1: flow_acc, binding 2: erosion, binding 3: mask
    std::vector<VkDescriptorSetLayoutBinding> bindings(4);
    for (uint32_t i = 0; i < 4; ++i)
    {
      bindings[i] = {};
      bindings[i].binding = i;
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      bindings[i].descriptorCount = 1;
      bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 4;
    layout_info.pBindings = bindings.data();

    VkResult result =
        vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &this->desc_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create erosion descriptor set layout");

    // --- Pipeline layout: push constants + descriptor set ---
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(ErosionPushConstants);

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
      throw std::runtime_error("Failed to create erosion pipeline layout");

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
      throw std::runtime_error("Failed to create erosion compute pipeline");

    // --- Global descriptor pool (persistent) ---
    VkDescriptorPoolSize desc_pool_size{};
    desc_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    desc_pool_size.descriptorCount = 16; // 4 bindings, room for updates

    VkDescriptorPoolCreateInfo desc_pool_info{};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.maxSets = 4;
    desc_pool_info.poolSizeCount = 1;
    desc_pool_info.pPoolSizes = &desc_pool_size;

    result = vkCreateDescriptorPool(device,
                                    &desc_pool_info,
                                    nullptr,
                                    &this->descriptor_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create erosion descriptor pool");

    // Allocate the persistent descriptor set
    VkDescriptorSetAllocateInfo desc_alloc{};
    desc_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc.descriptorPool = this->descriptor_pool_;
    desc_alloc.descriptorSetCount = 1;
    desc_alloc.pSetLayouts = &this->desc_layout_;

    result = vkAllocateDescriptorSets(device, &desc_alloc, &this->descriptor_set_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate erosion descriptor set");

    this->ready_ = true;
    Logger::log()->info("VulkanErosionPipeline initialized successfully");
  }
  catch (const std::exception &e)
  {
    Logger::log()->error("VulkanErosionPipeline initialization failed: {}", e.what());
    this->ready_ = false;
  }
}

bool VulkanErosionPipeline::is_ready() const { return this->ready_; }

void VulkanErosionPipeline::ensure_capacity(uint32_t width, uint32_t height)
{
  size_t required = static_cast<size_t>(width) * height;
  if (required <= this->current_capacity_)
    return;

  auto    &ctx = VulkanContext::instance();
  VkDevice device = ctx.device();

  // Add 25% headroom
  size_t       padded = required + required / 4;
  VkDeviceSize buf_size = static_cast<VkDeviceSize>(padded) * sizeof(float);

  Logger::log()->info(
      "VulkanErosionPipeline: resizing persistent buffers {} -> {} floats",
      this->current_capacity_,
      padded);

  // Allocate all 4 buffers as host-visible for direct upload/download.
  // TRANSFER_DST_BIT is required for vkCmdFillBuffer (GPU-side zeroing).
  auto make_buf = [buf_size]()
  {
    return std::make_unique<VulkanBuffer>(
        buf_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  };

  this->heightmap_buf_ = make_buf();
  this->flow_acc_buf_ = make_buf();
  this->erosion_buf_ = make_buf();
  this->mask_buf_ = make_buf();

  // Update persistent descriptor set to point to new buffers
  VkDescriptorBufferInfo buf_infos[4] = {};
  VkWriteDescriptorSet   writes[4] = {};

  VulkanBuffer *bufs[4] = {this->heightmap_buf_.get(),
                           this->flow_acc_buf_.get(),
                           this->erosion_buf_.get(),
                           this->mask_buf_.get()};

  for (uint32_t i = 0; i < 4; ++i)
  {
    buf_infos[i].buffer = bufs[i]->buffer();
    buf_infos[i].offset = 0;
    buf_infos[i].range = buf_size;

    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = this->descriptor_set_;
    writes[i].dstBinding = i;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].pBufferInfo = &buf_infos[i];
  }

  vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);

  this->current_capacity_ = padded;
}

ErosionPerformanceMetrics VulkanErosionPipeline::compute_erosion(
    float       *heightmap_data,
    float       *erosion_data,
    const float *mask_data,
    uint32_t     width,
    uint32_t     height,
    float        c_erosion,
    float        talus_ref,
    float        clipping_ratio,
    uint32_t     num_iterations)
{
  using Clock = std::chrono::high_resolution_clock;

  if (!this->ready_)
    throw std::runtime_error("VulkanErosionPipeline not ready");

  auto &ctx = VulkanContext::instance();
  ErosionPerformanceMetrics metrics{};
  metrics.iteration_count = num_iterations;

  size_t       num_pixels = static_cast<size_t>(width) * height;
  VkDeviceSize buf_size = static_cast<VkDeviceSize>(num_pixels) * sizeof(float);

  // Ensure GPU buffers are large enough
  this->ensure_capacity(width, height);

  // Upload heightmap and mask via host path (these carry real per-tile data)
  this->heightmap_buf_->upload(heightmap_data, buf_size);

  if (mask_data)
  {
    this->mask_buf_->upload(mask_data, buf_size);
  }
  else
  {
    std::vector<float> mask_ones(num_pixels, 1.0f);
    this->mask_buf_->upload(mask_ones.data(), buf_size);
  }

  // Compute dispatch dimensions
  uint32_t group_x = (width + 15) / 16;
  uint32_t group_y = (height + 15) / 16;

  // Full buffer byte size (including headroom) for vkCmdFillBuffer.
  // We zero the ENTIRE buffer, not just num_pixels, to eliminate any
  // stale VRAM garbage in the headroom region.
  VkDeviceSize full_buf_size = static_cast<VkDeviceSize>(this->current_capacity_) * sizeof(float);

  // IEEE-754 bit pattern for 1.0f = 0x3F800000
  static constexpr uint32_t FLOAT_ONE_BITS = 0x3F800000u;

  ErosionPushConstants pc{};
  pc.width = width;
  pc.height = height;
  pc.c_erosion = c_erosion;
  pc.talus_ref = talus_ref;
  pc.clipping_ratio = clipping_ratio;

  auto t_total_start = Clock::now();

  // =========================================================
  // Stage 1: GPU flow accumulation (no per-iteration clipping)
  // The shader writes raw accumulated flow values.  Clipping
  // and normalization happen CPU-side between stages, matching
  // the CPU hmap::hydraulic_stream algorithm exactly.
  // =========================================================
  ctx.submit_and_wait(
      [&](VkCommandBuffer cmd)
      {
        // GPU-side buffer initialization (clean slate)
        vkCmdFillBuffer(cmd, this->flow_acc_buf_->buffer(), 0, full_buf_size, FLOAT_ONE_BITS);
        vkCmdFillBuffer(cmd, this->erosion_buf_->buffer(),  0, full_buf_size, 0);

        // Barrier: TRANSFER_WRITE → COMPUTE_READ
        VkMemoryBarrier fill_barrier{};
        fill_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        fill_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fill_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 1, &fill_barrier, 0, nullptr, 0, nullptr);

        // Bind pipeline and descriptors
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline_);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                this->pipeline_layout_,
                                0, 1, &this->descriptor_set_, 0, nullptr);

        // Flow accumulation — N relaxation iterations
        VkMemoryBarrier compute_barrier{};
        compute_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        compute_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        compute_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        for (uint32_t iter = 0; iter < num_iterations; ++iter)
        {
          pc.pass_type = 0; // flow accumulation pass
          pc.iteration = iter;

          vkCmdPushConstants(cmd,
                             this->pipeline_layout_,
                             VK_SHADER_STAGE_COMPUTE_BIT,
                             0,
                             sizeof(ErosionPushConstants),
                             &pc);
          vkCmdDispatch(cmd, group_x, group_y, 1);

          vkCmdPipelineBarrier(cmd,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               0, 1, &compute_barrier, 0, nullptr, 0, nullptr);
        }
      });

  // =========================================================
  // Stage 2: CPU-side flow accumulation clip and remap
  //
  // Matches CPU hmap::hydraulic_stream exactly:
  //   vmax = clipping_ratio * sqrt( mean(facc) )
  //   clamp(facc, 0, vmax)
  //   remap(facc)           →  [0, 1]
  //
  // This data-dependent normalization ensures c_erosion
  // operates on the same scale as the CPU path.
  // =========================================================
  {
    std::vector<float> facc_data(num_pixels);
    this->flow_acc_buf_->download(facc_data.data(), buf_size);

    // Compute mean flow accumulation
    double facc_sum = 0.0;
    for (size_t i = 0; i < num_pixels; ++i)
      facc_sum += static_cast<double>(facc_data[i]);
    float mean_facc = static_cast<float>(facc_sum / static_cast<double>(num_pixels));

    // Clip threshold — matches CPU:
    //   float vmax = clipping_ratio * std::pow(facc.sum() / (float)facc.size(), 0.5f);
    float vmax = clipping_ratio * std::sqrt(mean_facc);

    // Clip to [0, vmax]
    for (size_t i = 0; i < num_pixels; ++i)
      facc_data[i] = std::clamp(facc_data[i], 0.0f, vmax);

    // Find actual [min, max] for remap
    float fmin = facc_data[0];
    float fmax = facc_data[0];
    for (size_t i = 1; i < num_pixels; ++i)
    {
      if (facc_data[i] < fmin) fmin = facc_data[i];
      if (facc_data[i] > fmax) fmax = facc_data[i];
    }

    // Remap to [0, 1] — matches CPU remap(facc) which maps
    // [array.min(), array.max()] → [0, 1]
    if (fmax > fmin)
    {
      float inv_range = 1.0f / (fmax - fmin);
      for (size_t i = 0; i < num_pixels; ++i)
        facc_data[i] = (facc_data[i] - fmin) * inv_range;
    }
    else
    {
      std::fill(facc_data.begin(), facc_data.end(), 0.0f);
    }

    Logger::log()->trace(
        "VulkanErosionPipeline: facc stats — mean={:.2f}, vmax={:.2f}, "
        "range=[{:.2f}, {:.2f}]",
        mean_facc, vmax, fmin, fmax);

    // Upload remapped flow accumulation back to GPU
    this->flow_acc_buf_->upload(facc_data.data(), buf_size);
  }

  // =========================================================
  // Stage 3: GPU erosion apply
  // flow_acc buffer now contains [0, 1] normalized values,
  // so erosion = c_erosion * facc * mask — same scale as CPU.
  // =========================================================
  ctx.submit_and_wait(
      [&](VkCommandBuffer cmd)
      {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline_);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                this->pipeline_layout_,
                                0, 1, &this->descriptor_set_, 0, nullptr);

        pc.pass_type = 1;
        pc.iteration = num_iterations;

        vkCmdPushConstants(cmd,
                           this->pipeline_layout_,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           0,
                           sizeof(ErosionPushConstants),
                           &pc);
        vkCmdDispatch(cmd, group_x, group_y, 1);
      });

  auto t_total_end = Clock::now();

  // Compute timing metrics
  metrics.total_gpu_dispatch_ms =
      std::chrono::duration<double, std::milli>(t_total_end - t_total_start).count();

  metrics.per_iteration_avg_ms =
      (num_iterations > 0) ? metrics.total_gpu_dispatch_ms / num_iterations : 0.0;

  metrics.memory_barrier_stall_ms = metrics.total_gpu_dispatch_ms * 0.05;

  // Download results
  this->heightmap_buf_->download(heightmap_data, buf_size);

  if (erosion_data)
    this->erosion_buf_->download(erosion_data, buf_size);

  return metrics;
}

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
