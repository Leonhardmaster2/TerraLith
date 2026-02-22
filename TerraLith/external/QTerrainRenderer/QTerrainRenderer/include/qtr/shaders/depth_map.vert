R""(
/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 3) in vec3 instance_pos;
layout(location = 4) in float instance_scale;
layout(location = 5) in float instance_rot;
layout(location = 6) in vec3 instance_color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool has_instances;

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

void main()
{
  // for instanced meshes only, adjust model matrix
  mat4 model_m = model;

  if (has_instances)
  {
    model_m = translate(model_m, instance_pos);
    model_m = rotate(model_m, instance_rot, vec3(0, 1, 0));
    model_m = scale(model_m, vec3(instance_scale));
  }

  gl_Position = projection * view * model_m * vec4(pos, 1.0);
}
)""
