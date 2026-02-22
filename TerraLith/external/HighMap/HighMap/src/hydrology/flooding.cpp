/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <limits>

#include "macrologger.h"

#include "highmap/array.hpp"
#include "highmap/erosion.hpp"
#include "highmap/features.hpp"
#include "highmap/filters.hpp"
#include "highmap/hydrology.hpp"
#include "highmap/interpolate2d.hpp"
#include "highmap/range.hpp"

namespace hmap
{

Array flooding_uniform_level(const Array &z, float zref)
{
  Array water_depth(z.shape, 0.f);

  water_depth = zref - z;
  clamp_min(water_depth, 0.f);

  return water_depth;
}

Array flooding_from_boundaries(const Array &z,
                               float        zref,
                               bool         from_east,
                               bool         from_west,
                               bool         from_north,
                               bool         from_south)
{
  Array water_depth = Array(z.shape);

  // find lowest points on the boundaries and starts the flooding there
  // std::vector<int> is, js;

  if (from_east)
  {
    int   i = z.shape.x - 1;
    float vmin = std::numeric_limits<float>::max();
    int   jmin = 0;

    for (int j = 0; j < z.shape.y - 1; ++j)
      if (z(i, j) < vmin)
      {
        vmin = z(i, j);
        jmin = j;
      }

    water_depth = flooding_from_point(z, i, jmin, zref - z(i, jmin));
  }

  if (from_west)
  {
    int   i = 0;
    float vmin = std::numeric_limits<float>::max();
    int   jmin = 0;

    for (int j = 0; j < z.shape.y - 1; ++j)
      if (z(i, j) < vmin)
      {
        vmin = z(i, j);
        jmin = j;
      }

    water_depth = maximum(water_depth,
                          flooding_from_point(z, i, jmin, zref - z(i, jmin)));
  }

  if (from_north)
  {
    int   j = z.shape.y - 1;
    float vmin = std::numeric_limits<float>::max();
    int   imin = 0;

    for (int i = 0; i < z.shape.x - 1; ++i)
      if (z(i, j) < vmin)
      {
        vmin = z(i, j);
        imin = i;
      }

    water_depth = maximum(water_depth,
                          flooding_from_point(z, imin, j, zref - z(imin, j)));
  }

  if (from_south)
  {
    int   j = 0;
    float vmin = std::numeric_limits<float>::max();
    int   imin = 0;

    for (int i = 0; i < z.shape.x - 1; ++i)
      if (z(i, j) < vmin)
      {
        vmin = z(i, j);
        imin = i;
      }

    water_depth = maximum(water_depth,
                          flooding_from_point(z, imin, j, zref - z(imin, j)));
  }

  return water_depth;
}

Array flooding_from_point(const Array &z,
                          const int    i,
                          const int    j,
                          float        depth_min)
{
  Array water_depth(z.shape, 0.f);

  std::vector<Vec2<int>> nbrs =
      {{-1, 0}, {0, 1}, {0, -1}, {1, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

  if (depth_min == std::numeric_limits<float>::max()) depth_min = 0.f;
  std::vector<Vec2<int>> queue = {{i, j}};

  // loop around the starting point, anything with elevation lower
  // than the reference elevation is water. If not, the cell is
  // outside the "water" mask
  while (queue.size() > 0)
  {
    Vec2<int> ij = queue.back();
    queue.pop_back();

    for (auto &idx : nbrs)
    {
      Vec2<int> pq = ij + idx;

      if (pq.x >= 0 && pq.x < z.shape.x && pq.y >= 0 && pq.y < z.shape.y)
      {
        float dz = z(i, j) + depth_min - z(pq.x, pq.y);
        if (z(pq.x, pq.y) < z(i, j) + depth_min && dz > water_depth(pq.x, pq.y))
        {
          water_depth(pq.x, pq.y) = dz;
          queue.push_back({pq.x, pq.y});
        }
      }
    }
  }

  return water_depth;
}

Array flooding_from_point(const Array            &z,
                          const std::vector<int> &i,
                          const std::vector<int> &j,
                          float                   depth_min)
{
  Array water_depth(z.shape);

  for (size_t k = 0; k < i.size(); k++)
    water_depth = maximum(water_depth,
                          flooding_from_point(z, i[k], j[k], depth_min));

  return water_depth;
}

Array flooding_lake_system(const Array &z,
                           int          iterations,
                           float        epsilon,
                           float        surface_threshold)
{
  Array water_depth = z;

  // use a rough depression filling algo to get the lake zones and depths
  depression_filling(water_depth, iterations, epsilon);

  for (int j = 0; j < z.shape.y; j++)
    for (int i = 0; i < z.shape.x; i++)
      water_depth(i, j) = std::max(0.f, water_depth(i, j) - z(i, j));

  // use a connected components analysis to remove small spots if
  // requested
  if (surface_threshold)
  {
    Array labels = connected_components(water_depth, surface_threshold);
    for (int j = 0; j < z.shape.y; j++)
      for (int i = 0; i < z.shape.x; i++)
        if (labels(i, j) == 0.f) water_depth(i, j) = 0.f;
  }

  return water_depth;
}

Array merge_water_depths(const Array &depth1,
                         const Array &depth2,
                         float        k_smooth)
{
  Array water_depth(depth1.shape);

  if (k_smooth == 0.f)
    water_depth = maximum(depth1, depth2);
  else
    water_depth = maximum_smooth(depth1, depth2, k_smooth) - k_smooth / 6.f;

  water_depth.infos();

  return water_depth;
}

void water_depth_dry_out(Array       &water_depth,
                         float        dry_out_ratio,
                         const Array *p_mask,
                         float        depth_max)
{
  if (depth_max == std::numeric_limits<float>::max())
    depth_max = water_depth.max();

  if (p_mask)
  {
    for (int j = 0; j < water_depth.shape.y; j++)
      for (int i = 0; i < water_depth.shape.x; i++)
      {
        water_depth(i, j) -= dry_out_ratio * depth_max * (*p_mask)(i, j);
        water_depth(i, j) = std::max(0.f, water_depth(i, j));
      }
  }
  else
  {
    for (int j = 0; j < water_depth.shape.y; j++)
      for (int i = 0; i < water_depth.shape.x; i++)
      {
        water_depth(i, j) -= dry_out_ratio * depth_max;
        water_depth(i, j) = std::max(0.f, water_depth(i, j));
      }
  }
}

Array water_depth_from_mask(const Array &z,
                            const Array &mask,
                            float        mask_threshold,
                            int          iterations_max,
                            float        tolerance,
                            float        omega)
{
  Array water_depth(z.shape);

  // transform to binary 0|1 mask
  Array mask_t = mask;
  make_binary(mask_t, mask_threshold);
  mask_t = 1.f - mask_t; // fixed values

  water_depth = harmonic_interpolation(z,
                                       mask_t,
                                       iterations_max,
                                       tolerance,
                                       omega) -
                z;

  return water_depth;
}

Array water_depth_increase(const Array &water_depth,
                           const Array &z,
                           float        additional_depth)
{
  Vec2<int> shape = water_depth.shape;
  Array     water_depth_extended(shape);

  size_t max_it = 2 * shape.x * shape.y; // failsafe
  size_t it;

  std::vector<Vec2<int>> nbrs =
      {{-1, 0}, {0, 1}, {0, -1}, {1, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

  // fill-in source pts
  std::vector<Vec2<int>> queue;

  for (int j = 0; j < shape.y; j++)
    for (int i = 0; i < shape.x; i++)
      if (water_depth(i, j) > 0.f)
      {
        water_depth_extended(i, j) = water_depth(i, j) + additional_depth;

        // check neighbors, only add border cells to the queue
        for (auto &idx : nbrs)
        {
          Vec2<int> pq = Vec2<int>(i, j) + idx;
          if (pq.x >= 0 && pq.x < shape.x && pq.y >= 0 && pq.y < shape.y)
          {
            if (water_depth(pq.x, pq.y) == 0.f)
            {
              queue.push_back({i, j});
              continue;
            }
          }
        }
      }

  // first pass - flood again, but only in upward direction
  it = 0;

  while (queue.size() > 0 && it < max_it)
  {
    Vec2<int> ij = queue.back();
    queue.pop_back();

    for (auto &idx : nbrs)
    {
      Vec2<int> pq = ij + idx;

      if (pq.x >= 0 && pq.x < shape.x && pq.y >= 0 && pq.y < shape.y)
      {
        float dz = z(pq.x, pq.y) - z(ij.x, ij.y);

        // upward only
        if (dz > 0.f)
        {
          float delta = water_depth_extended(ij.x, ij.y) - dz;

          if (delta > water_depth_extended(pq.x, pq.y))
          {
            water_depth_extended(pq.x, pq.y) = delta;
            queue.push_back({pq.x, pq.y});
          }
        }
      }
    }

    it++;
  }

  // second pass - fill holes
  queue.clear();

  for (int j = 0; j < shape.y; j++)
    for (int i = 0; i < shape.x; i++)
      if (water_depth_extended(i, j) > 0.f)
      {
        // check neighbors, add cells with discontinuity
        for (auto &idx : nbrs)
        {
          Vec2<int> pq = Vec2<int>(i, j) + idx;
          if (pq.x >= 0 && pq.x < shape.x && pq.y >= 0 && pq.y < shape.y)
          {
            if (water_depth_extended(pq.x, pq.y) == 0.f)
            {
              queue.push_back({i, j});
              continue;
            }
          }
        }
      }

  it = 0;

  while (queue.size() > 0 && it < max_it)
  {
    Vec2<int> ij = queue.back();
    queue.pop_back();

    for (auto &idx : nbrs)
    {
      Vec2<int> pq = ij + idx;

      if (pq.x >= 0 && pq.x < shape.x && pq.y >= 0 && pq.y < shape.y)
      {
        // if (water_depth_extended(pq.x, pq.y) == 0.f)
        {
          float dz = z(pq.x, pq.y) - z(ij.x, ij.y);
          float delta = water_depth_extended(ij.x, ij.y) + dz;

          if (dz < 0.f && delta > water_depth_extended(pq.x, pq.y))
          {
            water_depth_extended(pq.x, pq.y) = delta;
            queue.push_back({pq.x, pq.y});
          }
        }
      }
    }

    it++;
  }

  return water_depth_extended;
}

Array water_mask(const Array &water_depth)
{
  Array mask = water_depth;
  make_binary(mask);
  return mask;
}

Array water_mask(const Array &water_depth,
                 const Array &z,
                 float        additional_depth)
{
  Array mask(water_depth.shape);
  Array water_depth_extended = water_depth_increase(water_depth,
                                                    z,
                                                    additional_depth);

  mask = water_depth_extended - water_depth;
  mask /= additional_depth;
  mask = smoothstep3(mask);

  return mask;
}

} // namespace hmap
