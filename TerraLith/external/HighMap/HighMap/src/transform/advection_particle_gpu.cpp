/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/filters.hpp"
#include "highmap/gradient.hpp"
#include "highmap/hydrology.hpp"
#include "highmap/math.hpp"
#include "highmap/opencl/gpu_opencl.hpp"
#include "highmap/range.hpp"
#include "highmap/transform.hpp"

namespace hmap::gpu
{

Array advection_particle(const Array &z,
                         const Array &advected_field,
                         int          iterations,
                         int          nparticles,
                         uint         seed,
                         bool         reverse,
                         bool         post_filter,
                         float        post_filter_sigma,
                         float        advection_length,
                         float        value_persistence,
                         float        inertia,
                         const Array *p_advection_mask,
                         const Array *p_mask)
{
  Array out = advected_field;

  for (int it = 0; it < iterations; ++it)
    out = advection_particle(z,
                             out,
                             nparticles,
                             seed,
                             reverse,
                             post_filter,
                             post_filter_sigma,
                             advection_length,
                             value_persistence,
                             inertia,
                             p_advection_mask,
                             p_mask);

  return out;
}

Array advection_particle(const Array &z,
                         const Array &advected_field,
                         int          nparticles,
                         uint         seed,
                         bool         reverse,
                         bool         post_filter,
                         float        post_filter_sigma,
                         float        advection_length,
                         float        value_persistence,
                         float        inertia,
                         const Array *p_advection_mask,
                         const Array *p_mask)
{
  Array dx = -hmap::gradient_x(z);
  Array dy = -hmap::gradient_y(z);

  return advection_particle(dx,
                            dy,
                            advected_field,
                            nparticles,
                            seed,
                            reverse,
                            post_filter,
                            post_filter_sigma,
                            advection_length,
                            value_persistence,
                            inertia,
                            p_advection_mask,
                            p_mask);
}

Array advection_particle(const Array &dx,
                         const Array &dy,
                         const Array &advected_field,
                         int          nparticles,
                         uint         seed,
                         bool         reverse,
                         bool         post_filter,
                         float        post_filter_sigma,
                         float        advection_length,
                         float        value_persistence,
                         float        inertia,
                         const Array *p_advection_mask,
                         const Array *p_mask)
{
  auto run = clwrapper::Run("advection_particle");

  Vec2<int> shape = dx.shape;

  Array out(shape);
  Array count(shape);

  run.bind_buffer<float>("advected_field", advected_field.vector);
  run.bind_buffer<float>("dx", dx.vector);
  run.bind_buffer<float>("dy", dy.vector);
  run.bind_buffer<float>("out", out.vector);
  run.bind_buffer<float>("count", count.vector);
  helper_bind_optional_buffer(run, "advection_mask", p_advection_mask);

  run.bind_arguments(shape.x,
                     shape.y,
                     nparticles,
                     seed,
                     reverse ? -1.f : 1.f,
                     advection_length,
                     value_persistence,
                     inertia,
                     p_advection_mask ? 1 : 0);

  run.write_buffer("advected_field");
  run.write_buffer("dx");
  run.write_buffer("dy");

  run.execute(nparticles);

  run.read_buffer("out");
  run.read_buffer("count");

  for (int j = 0; j < shape.x; ++j)
    for (int i = 0; i < shape.y; ++i)
    {
      if (count(i, j))
      {
        out(i, j) += advected_field(i, j);
        out(i, j) /= (count(i, j) + 1.f);
      }
      else
      {
        out(i, j) = advected_field(i, j);
      }
    }

  // post-processing
  if (post_filter)
  {
    int max_it = 1;
    hmap::laplace(out, p_advection_mask, post_filter_sigma, max_it);
  }

  if (p_mask)
    return lerp(out, advected_field, *p_mask);
  else
    return out;
}

} // namespace hmap::gpu
