R""(
/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

float2 g_to_xy(const int2   g,
               const int    nx,
               const int    ny,
               const float  kx,
               const float  ky,
               const float  dx,
               const float  dy,
               const float4 bbox)
{
  float x = (float)g.x / (float)nx;
  float y = (float)g.y / (float)ny;

  x = kx * (x * (bbox.y - bbox.x) + bbox.x) + kx * dx;
  y = ky * (y * (bbox.w - bbox.z) + bbox.z) + ky * dy;

  return (float2)(x, y);
}

float2 g_to_xy_pixel_centered(const int2 g, const int nx, const int ny)
{
  float dx = 1.f / (float)nx;
  float dy = 1.f / (float)ny;

  float x = 0.5f * dx + (float)g.x * dx;
  float y = 0.5f * dy + (float)g.y * dy;
  return (float2)(x, y);
}

float2 g_to_xy_pixel_centered_bbox(const int2   g,
                                   const int    nx,
                                   const int    ny,
                                   const float4 bbox)
{
  float dx = 1.f / (float)nx;
  float dy = 1.f / (float)ny;

  float x = 0.5f * dx + (float)g.x * dx;
  float y = 0.5f * dy + (float)g.y * dy;

  x = x * (bbox.y - bbox.x) + bbox.x;
  y = y * (bbox.w - bbox.z) + bbox.z;

  return (float2)(x, y);
}

int2 xy_to_g(const float2 xy, const int nx, const int ny)
{
  return (int2)((int)(xy.x * (nx - 1.f)), (int)(xy.y * (ny - 1.f)));
}

bool is_inside(const int i, const int j, const int nx, const int ny)
{
  return i >= 0 && i < nx && j >= 0 && j < ny;
}

int linear_index(const int i, const int j, const int nx)
{
  return j * nx + i;
}

int linear_index_g(const int2 g, const int nx)
{
  return g.y * nx + g.x;
}

void update_interp_param(float2 pos, int *i, int *j, float *u, float *v)
{
  *i = (int)pos.x;
  *j = (int)pos.y;
  *u = pos.x - *i;
  *v = pos.y - *j;
}
)""
