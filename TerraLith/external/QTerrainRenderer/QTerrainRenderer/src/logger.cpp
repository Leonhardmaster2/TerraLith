/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/logger.hpp"

namespace qtr
{

// Initialize the static member
std::shared_ptr<spdlog::logger> Logger::instance = nullptr;

std::shared_ptr<spdlog::logger> &Logger::log()
{
  if (!instance)
  {
    instance = spdlog::stdout_color_mt("console_qtr");
    instance->set_pattern("[qtr---] [%H:%M:%S] [%^---%L---%$] %v");
    instance->set_level(spdlog::level::trace);
  }
  return instance;
}

} // namespace qtr
