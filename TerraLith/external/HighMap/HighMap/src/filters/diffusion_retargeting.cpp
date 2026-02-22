/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include "macrologger.h"

#include "highmap/boundary.hpp"
#include "highmap/filters.hpp"
#include "highmap/geometry/cloud.hpp"
#include "highmap/gradient.hpp"
#include "highmap/operator.hpp"
#include "highmap/range.hpp"

namespace hmap
{

Array diffusion_retargeting(const Array &array_before,
                            const Array &array_after,
                            int          ir)
{
  Vec2<int> shape = array_before.shape;

  // select points of interest
  Array delta(shape);

  for (int j = 1; j < shape.y - 1; j++)
    for (int i = 1; i < shape.x - 1; i++)
    {
      if (array_before(i, j) > array_before(i + 1, j) &&
          array_before(i, j) > array_before(i + 1, j + 1) &&
          array_before(i, j) > array_before(i, j + 1) &&
          array_before(i, j) > array_before(i - 1, j + 1) &&
          array_before(i, j) > array_before(i - 1, j) &&
          array_before(i, j) > array_before(i - 1, j - 1) &&
          array_before(i, j) > array_before(i, j - 1) &&
          array_before(i, j) > array_before(i + 1, j - 1))
      {
        delta(i, j) = array_before(i, j) - array_after(i, j);
      }
    }

  // apply correction
  float vmin = delta.min();
  float vmax = delta.max();

  smooth_cpulse(delta, ir);

  remap(delta, vmin, vmax);

  return delta + array_after;
}

Array diffusion_retargeting(const Array &array_before,
                            const Array &array_after,
                            const Array &mask,
                            int          iterations)
{
  Array error = mask * (array_before - array_after);
  Array error0 = error;

  for (int it = 0; it < iterations; ++it)
  {
    laplace(error, 0.125f, 1);
    error = (1.f - mask) * error0 + mask * error;
  }

  return array_after + error;
}

} // namespace hmap
