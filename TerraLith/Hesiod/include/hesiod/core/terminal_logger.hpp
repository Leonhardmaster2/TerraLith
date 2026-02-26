/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <chrono>
#include <string>

#include <spdlog/spdlog.h>

namespace hesiod
{

// Enhanced terminal logger for Hesiod 0.6.
// Provides color-coded output with Vulkan timing information
// and automatic stutter detection (>150 ms = warning).
class TerminalLogger
{
public:
  static TerminalLogger &instance();

  // Log graph compute start
  void log_graph_compute_started(int resolution, int tiling);

  // Log individual node compute with timing
  void log_node_compute(const std::string &node_name,
                        float              time_ms,
                        bool               is_vulkan,
                        bool               cache_hit = false,
                        int                omp_threads = 0);

  // Log graph compute completion with summary
  void log_graph_compute_finished(float total_ms, float cache_hit_rate);

  // Log stutter warning (>150 ms node compute)
  void log_stutter_warning(const std::string &node_name, float time_ms);

  // Log Vulkan-specific info
  void log_vulkan_info(const std::string &message);
  void log_vulkan_error(const std::string &message);

  // Settings
  void set_logging_level(int level); // 0=Silent, 1=Warning, 2=Info, 3=Debug, 4=Verbose
  void set_log_vulkan_timings(bool enabled);
  void set_show_stutter_warnings(bool enabled);

  int  get_logging_level() const { return logging_level_; }
  bool get_log_vulkan_timings() const { return log_vulkan_timings_; }
  bool get_show_stutter_warnings() const { return show_stutter_warnings_; }

private:
  TerminalLogger();
  TerminalLogger(const TerminalLogger &) = delete;
  TerminalLogger &operator=(const TerminalLogger &) = delete;

  int  logging_level_ = 2;           // Info
  bool log_vulkan_timings_ = true;
  bool show_stutter_warnings_ = true;
  float stutter_threshold_ms_ = 150.0f;
};

} // namespace hesiod
