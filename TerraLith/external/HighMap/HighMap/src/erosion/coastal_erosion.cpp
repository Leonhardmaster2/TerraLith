/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/array.hpp"
#include "highmap/filters.hpp"
#include "highmap/hydrology.hpp"
#include "highmap/morphology.hpp"
#include "highmap/range.hpp"

namespace hmap
{

void coastal_erosion_diffusion(Array &z,
                               Array &water_depth,
                               float  additional_depth,
                               int    iterations,
                               Array *p_water_mask)
{
  Array mask;
  Array z_bckp;

  for (int it = 0; it < iterations; ++it)
  {
    z_bckp = z;
    mask = water_mask(water_depth, z, additional_depth);

    // filtering
    hmap::laplace(z, &mask, /* sigma */ 0.125f, 1);

    // adjust water depth so that water height is the same as before
    // filtering
    for (int j = 0; j < z.shape.y; j++)
      for (int i = 0; i < z.shape.x; i++)
      {
        if (water_depth(i, j) > 0.f)
          water_depth(i, j) = z_bckp(i, j) + water_depth(i, j) - z(i, j);
      }
  }

  if (p_water_mask) *p_water_mask = mask;
}

void coastal_erosion_profile(Array &z,
                             Array &water_depth,
                             float  shore_ground_extent,
                             float  shore_water_extent,
                             float  slope_shore,
                             float  slope_shore_water,
                             float  scarp_extent_ratio,
                             bool   apply_post_filter,
                             Array *p_shore_mask)
{
  Array    z_bckp = z;
  Array    shore_mask(z.shape);  // includes ground & water
  Mat<int> closest_g_i(z.shape); // ground
  Mat<int> closest_g_j(z.shape);
  Mat<int> closest_w_i(z.shape); // water
  Mat<int> closest_w_j(z.shape);

  Array r_ground = distance_transform_with_closest(water_depth,
                                                   closest_g_i,
                                                   closest_g_j);
  Array r_water = distance_transform_with_closest(is_zero(water_depth),
                                                  closest_w_i,
                                                  closest_w_j);

  float slope_shore_n = slope_shore / float(z.shape.x);
  float slope_shore_water_n = slope_shore_water / float(z.shape.x);

  for (int j = 0; j < z.shape.y; ++j)
    for (int i = 0; i < z.shape.x; ++i)
    {
      if (r_ground(i, j) > 0.f)
      {
        // --- ground

        // transition factor
        float t = r_ground(i, j) / shore_ground_extent;

        if (t <= 1.f)
        {
          shore_mask(i, j) = 1.f - t;

          float t_scarp = 1.f - scarp_extent_ratio;
          float zref = z(closest_g_i(i, j), closest_g_j(i, j));
          float h = zref + slope_shore_n * r_ground(i, j);

          float new_z = 0.f;

          if (t < t_scarp)
          {
            // shore
            new_z = h;
          }
          else
          {
            // scarp
            float ts = (t - t_scarp) / (1.f - t_scarp); // in [0, 1]
            ts = smoothstep3(ts);

            new_z = lerp(h, z(i, j), ts);
          }

          z(i, j) = new_z;
        }
      }
      else
      {
        // --- underwater

        // transition factor
        float t = r_water(i, j) / shore_water_extent;

        if (t <= 1.f)
        {
          shore_mask(i, j) = 1.f - t;

          // ensure slope continuity at water level
          float slope = lerp(slope_shore_n, slope_shore_water_n, t);

          float zref = z(closest_w_i(i, j), closest_w_j(i, j));
          float h = zref - slope * r_water(i, j);
          float new_z = lerp(h, z(i, j), smoothstep3(t));

          z(i, j) = new_z;
        }
      }
    }

  if (apply_post_filter) laplace(z, &shore_mask);

  // adjust water depth so that water height is the same as before
  // filtering
  for (int j = 0; j < z.shape.y; j++)
    for (int i = 0; i < z.shape.x; i++)
    {
      if (water_depth(i, j) > 0.f)
        water_depth(i, j) = z_bckp(i, j) + water_depth(i, j) - z(i, j);
    }

  // other optional outputs
  if (p_shore_mask) *p_shore_mask = shore_mask;
}

void coastal_erosion_profile(Array       &z,
                             const Array *p_mask,
                             Array       &water_depth,
                             float        shore_ground_extent,
                             float        shore_water_extent,
                             float        slope_shore,
                             float        slope_shore_water,
                             float        scarp_extent_ratio,
                             bool         apply_post_filter,
                             Array       *p_shore_mask)
{
  if (!p_mask)
    coastal_erosion_profile(z,
                            water_depth,
                            shore_ground_extent,
                            shore_water_extent,
                            slope_shore,
                            slope_shore_water,
                            scarp_extent_ratio,
                            apply_post_filter,
                            p_shore_mask);
  else
  {
    Array z_f = z;
    coastal_erosion_profile(z_f,
                            water_depth,
                            shore_ground_extent,
                            shore_water_extent,
                            slope_shore,
                            slope_shore_water,
                            scarp_extent_ratio,
                            apply_post_filter,
                            p_shore_mask);
    z = lerp(z, z_f, *(p_mask));
  }
}

} // namespace hmap
