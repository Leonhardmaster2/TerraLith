/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#ifdef HESIOD_HAS_VULKAN

#include <cstddef>

namespace hesiod
{

class VulkanComputeTest
{
public:
  static bool run_add_test(size_t num_elements = 1024);
};

} // namespace hesiod

#endif // HESIOD_HAS_VULKAN
