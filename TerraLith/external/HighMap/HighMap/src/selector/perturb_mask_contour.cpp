/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/array.hpp"
#include "highmap/morphology.hpp"

namespace hmap
{

Array perturb_mask_contour(const Array &mask,
                           const Array &noise,
                           float        max_displacement,
                           int          ir)
{
  Vec2<int> shape = mask.shape;

  Array mask_out(shape);
  Array boundary = border(mask, 1);

  for (int j = 1; j < shape.y - 1; j++)
    for (int i = 1; i < shape.x - 1; i++)
    {
      if (boundary(i, j) == 0.f) continue;

      // compute mask normal at boundary
      Vec2<float> dir = {0.f, 0.f};

      for (int p = -ir; p <= ir; ++p)
        for (int q = -ir; q <= ir; ++q)
        {
          if (p == 0 && q == 0) continue;

          if (mask(i + p, j + q) > 0.f)
          {
            float n = std::sqrt(p * p + q * q);
            dir = dir + Vec2<float>(float(p) / n, float(q) / n);
          }
        }

      dir.normalize();

      // apply
      float dn = max_displacement * noise(i, j);
      int   new_i = std::clamp(int(i - dn * dir.x), 0, shape.x - 1);
      int   new_j = std::clamp(int(j - dn * dir.y), 0, shape.y - 1);

      mask_out(new_i, new_j) = 1.f;
    }

  // fill discontinuities
  mask_out = dilation(mask_out, ir);

  // fill-in contour
  flood_fill(mask_out, 0, 0);

  return 1.f - mask_out;
}

} // namespace hmap
