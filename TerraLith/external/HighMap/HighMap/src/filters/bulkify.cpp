/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/math.hpp"
#include "highmap/operator.hpp"
#include "highmap/primitives.hpp"

namespace hmap
{

Array bulkify(const Array         &z,
              const PrimitiveType &primitive_type,
              float                amp,
              const Array         *p_noise_x,
              const Array         *p_noise_y,
              Vec2<float>          center,
              Vec4<float>          bbox)
{
  return z + amp * get_primitive_base(primitive_type,
                                      z.shape,
                                      p_noise_x,
                                      p_noise_y,
                                      center,
                                      bbox);
}

} // namespace hmap
