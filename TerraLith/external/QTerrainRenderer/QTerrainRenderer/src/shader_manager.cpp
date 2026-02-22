/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/shader_manager.hpp"
#include "qtr/logger.hpp"

namespace qtr
{

ShaderManager::~ShaderManager()
{
  qtr::Logger::log()->trace("ShaderManager::~ShaderManager");
  this->clear();
}

bool ShaderManager::add_shader_from_code(const std::string &name,
                                         const std::string &vertex_code,
                                         const std::string &fragment_code)
{
  qtr::Logger::log()->trace("ShaderManager::add_shader_from_code: {}", name);

  auto shader = std::make_unique<Shader>();

  if (!shader->from_code(vertex_code, fragment_code))
  {
    qtr::Logger::log()->error(
        "ShaderManager::add_shader_from_code: could not add shader");
    return false;
  }

  this->shaders[name] = std::move(shader);
  return true;
}

bool ShaderManager::add_shader_from_file(const std::string &name,
                                         const std::string &vertex_path,
                                         const std::string &fragment_path)
{
  qtr::Logger::log()->trace("ShaderManager::add_shader_from_file: {}", name);

  auto shader = std::make_unique<Shader>();

  if (!shader->from_file(vertex_path, fragment_path))
  {
    qtr::Logger::log()->error(
        "ShaderManager::add_shader_from_file: could not add shader");
    return false;
  }

  this->shaders[name] = std::move(shader);
  return true;
}

Shader *ShaderManager::get(const std::string &name)
{
  if (this->shaders.contains(name))
    return this->shaders.at(name).get();
  else
  {
    qtr::Logger::log()->error("unknown shader: {}", name);
    return nullptr;
  }
}

void ShaderManager::clear() { this->shaders.clear(); }

} // namespace qtr
