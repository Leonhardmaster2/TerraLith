/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/boundary.hpp"
#include "highmap/curvature.hpp"
#include "highmap/filters.hpp"
#include "highmap/gradient.hpp"
#include "highmap/range.hpp"

namespace hmap::gpu
{

Array accumulation_curvature(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::accumulation_curvature(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_horizontal_cross_sectional(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_horizontal_cross_sectional(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_horizontal_plan(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_horizontal_plan(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_horizontal_tangential(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_horizontal_tangential(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_ring(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_ring(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_rotor(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_rotor(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_vertical_longitudinal(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_vertical_longitudinal(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array curvature_vertical_profile(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::curvature_vertical_profile(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array level_set_curvature(const Array &array, int prefilter_ir)
{
  Array array_f = array;
  if (prefilter_ir) gpu::smooth_cpulse(array_f, prefilter_ir);

  // compute divergence of the normalized gradient
  Array gx = gradient_x(array_f);
  Array gy = gradient_y(array_f);
  Array gn = hmap::gradient_norm(array_f) + 1e-12f;

  Array dgx = gradient_x(gx / gn);
  Array dgy = gradient_y(gy / gn);

  gn = dgx + dgy;

  return gn;
}

Array shape_index(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::shape_index(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

Array unsphericity(const Array &z, int ir)
{
  Array c = z;
  if (ir > 0) gpu::smooth_cpulse(c, ir);

  c = hmap::unsphericity(c, 0);

  set_borders(c, 0.f, ir);
  return c;
}

} // namespace hmap::gpu
