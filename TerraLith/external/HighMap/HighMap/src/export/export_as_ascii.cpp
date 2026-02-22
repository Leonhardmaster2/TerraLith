/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "macrologger.h"

#include "highmap/array.hpp"
#include "highmap/range.hpp"

namespace hmap
{

std::string export_as_ascii(const Array      &array,
                            const Vec2<int>  &export_shape,
                            const std::string chars_map)
{
  std::string out = "";
  size_t      nc = chars_map.size();

  Array array_export = array.resample_to_shape_nearest(export_shape);
  remap(array_export);

  for (int j = array_export.shape.y - 1; j >= 0; j--)
  {
    for (int i = 0; i < array_export.shape.x; ++i)
    {
      size_t index = static_cast<size_t>(array_export(i, j) * (nc - 1));
      out += chars_map[index];
    }
    out += "\n";
  }

  return out;
}

} // namespace hmap
