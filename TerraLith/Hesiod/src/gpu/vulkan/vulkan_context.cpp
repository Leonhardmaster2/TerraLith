/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#ifdef HESIOD_HAS_VULKAN

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "hesiod/gpu/vulkan/vulkan_context.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

// --- Debug callback ---

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
               VkDebugUtilsMessageTypeFlagsEXT             /*type*/,
               const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
               void * /*user_data*/)
{
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    Logger::log()->warn("Vulkan validation: {}", callback_data->pMessage);
  else
    Logger::log()->trace("Vulkan validation: {}", callback_data->pMessage);
  return VK_FALSE;
}

// --- Singleton ---

VulkanContext &VulkanContext::instance()
{
  static VulkanContext ctx;
  return ctx;
}

VulkanContext::VulkanContext() { this->init(); }

VulkanContext::~VulkanContext()
{
  if (this->device_ != VK_NULL_HANDLE)
    vkDeviceWaitIdle(this->device_);

  if (this->command_pool_ != VK_NULL_HANDLE)
    vkDestroyCommandPool(this->device_, this->command_pool_, nullptr);

  if (this->device_ != VK_NULL_HANDLE)
    vkDestroyDevice(this->device_, nullptr);

  if (this->debug_messenger_ != VK_NULL_HANDLE)
  {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(this->instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (func)
      func(this->instance_, this->debug_messenger_, nullptr);
  }

  if (this->instance_ != VK_NULL_HANDLE)
    vkDestroyInstance(this->instance_, nullptr);

  Logger::log()->trace("VulkanContext destroyed");
}

// --- Initialization ---

void VulkanContext::init()
{
  try
  {
    this->create_instance();
    this->select_physical_device();
    this->create_logical_device();
    this->create_command_pool();
    this->ready_ = true;
    Logger::log()->info("VulkanContext initialized successfully");
  }
  catch (const std::exception &e)
  {
    Logger::log()->error("VulkanContext initialization failed: {}", e.what());
    this->ready_ = false;
  }
}

void VulkanContext::create_instance()
{
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Hesiod";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 5, 0);
  app_info.pEngineName = "Hesiod Compute";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  std::vector<const char *> extensions;
  std::vector<const char *> layers;

#ifndef NDEBUG
  // Check if validation layer is available
  uint32_t layer_count = 0;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  bool validation_available = false;
  for (const auto &layer : available_layers)
  {
    if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
    {
      validation_available = true;
      break;
    }
  }

  if (validation_available)
  {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    Logger::log()->trace("Vulkan validation layers enabled");
  }
  else
  {
    Logger::log()->warn("Vulkan validation layers not available");
  }
#endif

  create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
  create_info.ppEnabledLayerNames = layers.data();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  VkResult result = vkCreateInstance(&create_info, nullptr, &this->instance_);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan instance, error: " +
                             std::to_string(result));

#ifndef NDEBUG
  if (validation_available)
  {
    VkDebugUtilsMessengerCreateInfoEXT dbg_info{};
    dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_info.pfnUserCallback = debug_callback;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(this->instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (func)
      func(this->instance_, &dbg_info, nullptr, &this->debug_messenger_);
  }
#endif

  Logger::log()->trace("Vulkan instance created");
}

void VulkanContext::select_physical_device()
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(this->instance_, &device_count, nullptr);

  if (device_count == 0)
    throw std::runtime_error("No Vulkan-capable GPU found");

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(this->instance_, &device_count, devices.data());

  // Prefer discrete GPU
  VkPhysicalDevice fallback = VK_NULL_HANDLE;

  for (const auto &dev : devices)
  {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(dev, &props);

    // Check for compute queue support
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qf_props(qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_count, qf_props.data());

    bool has_compute = false;
    for (const auto &qf : qf_props)
    {
      if (qf.queueFlags & VK_QUEUE_COMPUTE_BIT)
      {
        has_compute = true;
        break;
      }
    }

    if (!has_compute)
      continue;

    Logger::log()->trace("Vulkan device found: {} (type {})",
                         props.deviceName,
                         static_cast<int>(props.deviceType));

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      this->physical_device_ = dev;
      Logger::log()->info("Selected discrete GPU: {}", props.deviceName);
      return;
    }

    if (fallback == VK_NULL_HANDLE)
      fallback = dev;
  }

  if (fallback != VK_NULL_HANDLE)
  {
    this->physical_device_ = fallback;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(fallback, &props);
    Logger::log()->info("Selected GPU (fallback): {}", props.deviceName);
  }
  else
  {
    throw std::runtime_error("No suitable Vulkan GPU found");
  }
}

void VulkanContext::create_logical_device()
{
  this->queue_family_ = this->find_compute_queue_family();

  float queue_priority = 1.0f;

  VkDeviceQueueCreateInfo queue_info{};
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.queueFamilyIndex = this->queue_family_;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceFeatures device_features{};

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_info;
  create_info.pEnabledFeatures = &device_features;

  VkResult result =
      vkCreateDevice(this->physical_device_, &create_info, nullptr, &this->device_);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan logical device, error: " +
                             std::to_string(result));

  vkGetDeviceQueue(this->device_, this->queue_family_, 0, &this->compute_queue_);
  Logger::log()->trace("Vulkan logical device and compute queue created");
}

void VulkanContext::create_command_pool()
{
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = this->queue_family_;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkResult result =
      vkCreateCommandPool(this->device_, &pool_info, nullptr, &this->command_pool_);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan command pool, error: " +
                             std::to_string(result));

  Logger::log()->trace("Vulkan command pool created");
}

// --- Helpers ---

uint32_t VulkanContext::find_compute_queue_family() const
{
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(this->physical_device_, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(this->physical_device_,
                                           &count,
                                           families.data());

  // Prefer a dedicated compute queue (compute but not graphics)
  for (uint32_t i = 0; i < count; ++i)
  {
    if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
        !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
      return i;
  }

  // Fall back to any queue with compute
  for (uint32_t i = 0; i < count; ++i)
  {
    if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
      return i;
  }

  throw std::runtime_error("No compute queue family found");
}

uint32_t VulkanContext::find_memory_type(uint32_t              type_filter,
                                         VkMemoryPropertyFlags properties) const
{
  VkPhysicalDeviceMemoryProperties mem_props;
  vkGetPhysicalDeviceMemoryProperties(this->physical_device_, &mem_props);

  for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
  {
    if ((type_filter & (1 << i)) &&
        (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  }

  throw std::runtime_error("Failed to find suitable Vulkan memory type");
}

void VulkanContext::submit_and_wait(
    const std::function<void(VkCommandBuffer)> &record_fn)
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = this->command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(this->device_, &alloc_info, &cmd);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &begin_info);

  record_fn(cmd);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  VkFence fence;
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  vkCreateFence(this->device_, &fence_info, nullptr, &fence);

  vkQueueSubmit(this->compute_queue_, 1, &submit_info, fence);
  vkWaitForFences(this->device_, 1, &fence, VK_TRUE, UINT64_MAX);

  vkDestroyFence(this->device_, fence, nullptr);
  vkFreeCommandBuffers(this->device_, this->command_pool_, 1, &cmd);
}

// --- Accessors ---

bool             VulkanContext::is_ready() const { return this->ready_; }
VkDevice         VulkanContext::device() const { return this->device_; }
VkPhysicalDevice VulkanContext::physical_device() const { return this->physical_device_; }
VkQueue          VulkanContext::compute_queue() const { return this->compute_queue_; }
uint32_t         VulkanContext::compute_queue_family() const { return this->queue_family_; }
VkInstance       VulkanContext::vk_instance() const { return this->instance_; }

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
