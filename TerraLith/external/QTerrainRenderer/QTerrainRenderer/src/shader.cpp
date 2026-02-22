/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <fstream>

#include "qtr/logger.hpp"
#include "qtr/shader.hpp"

namespace qtr
{

Shader::~Shader() { this->destroy(); }

bool Shader::from_code(const std::string &vertex_code, const std::string &fragment_code)
{
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();

  this->destroy();
  this->sp_program = std::make_unique<QOpenGLShaderProgram>();

  if (!this->sp_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                 vertex_code.c_str()))
  {
    qtr::Logger::log()->error(
        "Shader::from_code: could not add vertex shader source code");
    qtr::Logger::log()->error("Shader::from_code: build log >>>");
    qtr::Logger::log()->error("{}", this->sp_program->log().toStdString());
    qtr::Logger::log()->error("Shader::from_code: <<< build log");
    return false;
  }

  if (!this->sp_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                 fragment_code.c_str()))
  {
    qtr::Logger::log()->error("Shader::from_code: could not add fragment source code");
    qtr::Logger::log()->error("Shader::from_code: build log >>>");
    qtr::Logger::log()->error("{}", this->sp_program->log().toStdString());
    qtr::Logger::log()->error("Shader::from_code: <<< build log");
    return false;
  }

  if (!this->sp_program->link())
  {
    qtr::Logger::log()->error("Shader::from_code: could not link shader program");
    qtr::Logger::log()->error("Shader::from_code: build log >>>");
    qtr::Logger::log()->error("{}", this->sp_program->log().toStdString());
    qtr::Logger::log()->error("Shader::from_code: <<< build log");
    return false;
  }

  return true;
}

bool Shader::from_file(const std::string &vertex_path, const std::string &fragment_path)
{
  // QOpenGLFunctions_3_3_Core::initializeOpenGLFunctions();

  // Helper lambda to read file contents into a string
  auto read_file = [](const std::string &path) -> std::string
  {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
      qtr::Logger::log()->error("Shader::from_file: cannot open file '{}'", path);
      return {};
    }

    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
  };

  // Read vertex and fragment shader sources
  std::string vertex_code = read_file(vertex_path);
  if (vertex_code.empty())
  {
    qtr::Logger::log()->error("Shader::from_file: vertex shader '{}' is empty",
                              vertex_path);
    return false;
  }

  std::string fragment_code = read_file(fragment_path);
  if (fragment_code.empty())
  {
    qtr::Logger::log()->error("Shader::from_file: fragment shader '{}' is empty",
                              fragment_path);
    return false;
  }

  // Compile and link using the other method
  return this->from_code(vertex_code, fragment_code);
}

void Shader::destroy() { this->sp_program.reset(); }

QOpenGLShaderProgram *Shader::get() { return this->sp_program.get(); }

const QOpenGLShaderProgram *Shader::get() const { return this->sp_program.get(); }

} // namespace qtr
