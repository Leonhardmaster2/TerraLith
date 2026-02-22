/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/array.hpp"

namespace hmap
{

void solve_poisson_gauss_seidel(const Array &rhs,
                                Array       &h,
                                int          iterations,
                                float        omega)
{
  int nx = rhs.shape.x;
  int ny = rhs.shape.y;

  for (int it = 0; it < iterations; ++it)
    for (int j = 1; j < ny - 1; ++j)
      for (int i = 1; i < nx - 1; ++i)
      {
        float new_val = 0.25f * (h(i + 1, j) + h(i - 1, j) + h(i, j + 1) +
                                 h(i, j - 1) - rhs(i, j));

        h(i, j) = h(i, j) + omega * (new_val - h(i, j));
      }
}

} // namespace hmap
