R""(
/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#version 330 core

in vec3 frag_normal;
in vec3 frag_pos;
in vec2 frag_uv;

out vec4 frag_color;

uniform vec3  color;         // Base color of the object
uniform vec3  light_dir;     // Direction *towards* the light
uniform vec3  view_pos;      // Camera position in world space
uniform float shininess;     // Controls specular sharpness
uniform float spec_strength; // Controls specular intensity

void main()
{
  vec3 norm = normalize(frag_normal);
  vec3 light = normalize(-light_dir);
  vec3 view_dir = normalize(view_pos - frag_pos);

  // --- Diffuse ---
  float diff = max(dot(norm, light), 0.0);
  vec3  diffuse = color * diff;

  // --- Specular (Phong) ---
  vec3  reflect_dir = reflect(-light, norm);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
  vec3  specular = spec_strength * spec * vec3(1.0);

  // --- Ambient ---
  vec3 ambient = 0.2 * color;

  // Combine results
  vec3 result = ambient + diffuse + specular;
  frag_color = vec4(result, 1.0);
}
)""
