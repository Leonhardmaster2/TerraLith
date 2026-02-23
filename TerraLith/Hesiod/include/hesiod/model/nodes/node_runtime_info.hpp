/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <chrono>
#include <string>

#include "nlohmann/json.hpp"

namespace hesiod
{

enum class ComputeBackend : int
{
  NONE = 0,
  CPU = 1,
  VULKAN = 2,
  OPENCL = 3
};

enum class NodeRuntimeStep
{
  NRS_INIT,
  NRS_UPDATE_START,
  NRS_UPDATE_END
};

struct NodeRuntimeInfo
{
  std::chrono::system_clock::time_point time_creation;
  std::chrono::system_clock::time_point time_last_update;
  float                                 update_time;
  size_t                                eval_count = 0;
  ComputeBackend                        last_backend_used = ComputeBackend::NONE;

  virtual void           json_from(nlohmann::json const &json);
  virtual nlohmann::json json_to() const;

  std::chrono::steady_clock::time_point timer_t0;
};

} // namespace hesiod