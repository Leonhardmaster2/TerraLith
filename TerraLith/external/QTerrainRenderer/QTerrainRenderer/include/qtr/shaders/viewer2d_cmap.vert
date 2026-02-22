R""(
/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#version 330 core

// ============================================================================
// Inputs / Outputs
// ============================================================================

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 instance_pos;
layout(location = 4) in float instance_scale;
layout(location = 5) in float instance_rot;
layout(location = 6) in vec3 instance_color;

out vec3 frag_pos;
out vec3 frag_normal;
out vec2 frag_uv;
out vec3 frag_instance_color;

// ============================================================================
// Uniforms
// ============================================================================

uniform mat4  model;
uniform float aspect_ratio;
uniform float zoom;

uniform bool has_instances;

// ============================================================================
// Utility Functions
// ============================================================================

mat4 translate(mat4 m, vec3 v)
{
  mat4 t = mat4(1.0);
  t[3] = vec4(v, 1.0);
  return m * t;
}

mat4 scale(mat4 m, vec3 v)
{
  mat4 s = mat4(1.0);
  s[0][0] = v.x;
  s[1][1] = v.y;
  s[2][2] = v.z;
  return m * s;
}

mat4 rotate(mat4 m, float angle, vec3 axis)
{
  axis = normalize(axis);
  float c = cos(angle);
  float s = sin(angle);
  float omc = 1.0 - c;

  mat4 r = mat4(1.0);
  r[0][0] = axis.x * axis.x * omc + c;
  r[0][1] = axis.x * axis.y * omc + axis.z * s;
  r[0][2] = axis.x * axis.z * omc - axis.y * s;

  r[1][0] = axis.y * axis.x * omc - axis.z * s;
  r[1][1] = axis.y * axis.y * omc + c;
  r[1][2] = axis.y * axis.z * omc + axis.x * s;

  r[2][0] = axis.z * axis.x * omc + axis.y * s;
  r[2][1] = axis.z * axis.y * omc - axis.x * s;
  r[2][2] = axis.z * axis.z * omc + c;

  return m * r;
}

// ============================================================================
// Main
// ============================================================================

void main()
{
  // adjust model matrix, but for instanced meshes only
  mat4 model_m = model;

  if (has_instances)
  {
    frag_instance_color = instance_color; // pass through

    model_m = translate(model_m, instance_pos);
    model_m = rotate(model_m, instance_rot, vec3(0, 1, 0));
    model_m = scale(model_m, vec3(instance_scale));
  }

  vec4 world = model_m * vec4(pos, 1.0);

  // top view: X stays X, Z becomes Y, flatten Y

  // TODO profile view option

  frag_pos = vec3(zoom * world.x / aspect_ratio, zoom * world.z, 0.0);
  frag_normal = mat3(transpose(inverse(model_m))) * normal;
  frag_uv = uv;

  gl_Position = vec4(frag_pos, 1.0);
}
)""
