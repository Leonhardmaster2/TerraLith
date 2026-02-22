R""(
/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#version 330 core

in vec3 frag_normal;
in vec3 frag_pos;
in vec2 frag_uv;

out vec4 frag_color;

uniform vec3 color;
uniform vec3 light_dir;

void main()
{
  // Normalize the interpolated normal
  vec3 norm = normalize(frag_normal);
  vec3 light = normalize(light_dir);

  // Basic diffuse shading
  float diff = max(dot(norm, light), 0.0);
  vec3  base_color = color * (0.2 + 0.8 * diff);

  frag_color = vec4(base_color, 1.0);
}
)""
