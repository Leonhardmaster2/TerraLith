/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "macrologger.h"

#include "highmap/array.hpp"

namespace hmap
{

Array harmonic_interpolation(const Array &array,
                             const Array &mask_fixed_values,
                             int          iterations_max,
                             float        tolerance,
                             float        omega)
{
  Array out = array;
  int   nx = out.shape.x;
  int   ny = out.shape.y;

  for (int it = 0; it < iterations_max; ++it)
  {
    float max_diff = 0.f;

    // sweep over interior
    for (int i = 1; i < nx - 1; ++i)
      for (int j = 1; j < ny - 1; ++j)
      {
        if (mask_fixed_values(i, j) > 0.f) continue;

        float new_val = 0.25f * (out(i - 1, j) + out(i + 1, j) + out(i, j - 1) +
                                 out(i, j + 1));
        float diff = new_val - out(i, j);
        out(i, j) += omega * diff; // SOR update
        max_diff = std::max(max_diff, std::abs(diff));
      }

    if (max_diff < tolerance) break;
  }

  return out;
}

} // namespace hmap
