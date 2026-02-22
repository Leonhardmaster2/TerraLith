/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <map>

#include "qtr/shader.hpp"

namespace qtr
{

class ShaderManager
{
public:
  ShaderManager() = default;
  ~ShaderManager();

  bool add_shader_from_code(const std::string &name,
                            const std::string &vertex_code,
                            const std::string &fragment_code);
  bool add_shader_from_file(const std::string &name,
                            const std::string &vertex_path,
                            const std::string &fragment_path);

  void    clear();
  Shader *get(const std::string &name);

private:
  std::map<std::string, std::unique_ptr<Shader>> shaders;
};

} // namespace qtr