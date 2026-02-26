/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <format>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "hesiod/core/terminal_logger.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

TerminalLogger::TerminalLogger()
{
  // Defaults set in header
}

TerminalLogger &TerminalLogger::instance()
{
  static TerminalLogger inst;
  return inst;
}

void TerminalLogger::log_graph_compute_started(int resolution, int tiling)
{
  if (logging_level_ < 2) // Info
    return;

  Logger::log()->info("[INFO] Graph compute started ({}x{}, {}x{})",
                      resolution,
                      resolution,
                      tiling,
                      tiling);
}

void TerminalLogger::log_node_compute(const std::string &node_name,
                                      float              time_ms,
                                      bool               is_vulkan,
                                      bool               cache_hit,
                                      int                omp_threads)
{
  if (logging_level_ < 2)
    return;

  std::string backend = is_vulkan ? "VULKAN" : "CPU";
  std::string suffix;

  if (cache_hit)
    suffix = "(hit)";
  else if (!is_vulkan && omp_threads > 0)
    suffix = std::format("(OpenMP {})", omp_threads);

  if (log_vulkan_timings_ || !is_vulkan)
  {
    Logger::log()->info("[{}] {} -> {:.0f} ms {}",
                        backend,
                        node_name,
                        time_ms,
                        suffix);
  }

  // Stutter detection
  if (show_stutter_warnings_ && time_ms > stutter_threshold_ms_)
  {
    log_stutter_warning(node_name, time_ms);
  }
}

void TerminalLogger::log_graph_compute_finished(float total_ms, float cache_hit_rate)
{
  if (logging_level_ < 2)
    return;

  Logger::log()->info("[INFO] Graph complete {:.0f} ms (cache {:.0f} %)",
                      total_ms,
                      cache_hit_rate);
}

void TerminalLogger::log_stutter_warning(const std::string &node_name, float time_ms)
{
  if (logging_level_ < 1) // Warning
    return;

  Logger::log()->warn("[WARN] Stutter: {} {:.0f} ms — consider lower iterations",
                      node_name,
                      time_ms);
}

void TerminalLogger::log_vulkan_info(const std::string &message)
{
  if (logging_level_ < 2)
    return;

  Logger::log()->info("[VULKAN] {}", message);
}

void TerminalLogger::log_vulkan_error(const std::string &message)
{
  Logger::log()->error("[VULKAN ERROR] {}", message);
}

void TerminalLogger::set_logging_level(int level)
{
  logging_level_ = level;

  // Map to spdlog levels
  switch (level)
  {
  case 0: spdlog::set_level(spdlog::level::off); break;
  case 1: spdlog::set_level(spdlog::level::warn); break;
  case 2: spdlog::set_level(spdlog::level::info); break;
  case 3: spdlog::set_level(spdlog::level::debug); break;
  case 4: spdlog::set_level(spdlog::level::trace); break;
  default: spdlog::set_level(spdlog::level::info); break;
  }
}

void TerminalLogger::set_log_vulkan_timings(bool enabled)
{
  log_vulkan_timings_ = enabled;
}

void TerminalLogger::set_show_stutter_warnings(bool enabled)
{
  show_stutter_warnings_ = enabled;
}

} // namespace hesiod
