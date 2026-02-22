/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "macrologger.h"

#include "highmap/curvature.hpp"
#include "highmap/filters.hpp"
#include "highmap/gradient.hpp"
#include "highmap/morphology.hpp"

namespace hmap
{

Array signed_curvature_from_distance(const Array &array, int prefilter_ir)
{
  Array dist = distance_transform(array);
  return level_set_curvature(dist, prefilter_ir);
}

Array signed_distance_transform(const Array &array, int prefilter_ir)
{
  Array dist0 = distance_transform(array);
  Array dist = dist0;
  Array gn = level_set_curvature(dist, prefilter_ir);

  for (int j = 0; j < array.shape.y; ++j)
    for (int i = 0; i < array.shape.x; ++i)
      dist0(i, j) *= std::copysign(1.f, gn(i, j));

  return dist0;
}

} // namespace hmap

namespace hmap::gpu
{

Array signed_curvature_from_distance(const Array &array, int prefilter_ir)
{
  Array dist = distance_transform(array);
  return gpu::level_set_curvature(dist, prefilter_ir);
}

Array signed_distance_transform(const Array &array, int prefilter_ir)
{
  Array dist0 = distance_transform(array);
  Array dist = dist0;
  Array gn = gpu::level_set_curvature(dist, prefilter_ir);

  for (int j = 0; j < array.shape.y; ++j)
    for (int i = 0; i < array.shape.x; ++i)
      dist0(i, j) *= std::copysign(1.f, gn(i, j));

  return dist0;
}

} // namespace hmap::gpu
