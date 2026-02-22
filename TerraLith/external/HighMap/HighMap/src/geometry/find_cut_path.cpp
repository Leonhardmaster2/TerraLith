/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/geometry/path.hpp"
#include "highmap/primitives.hpp"
#include "highmap/shortest_path.hpp"

namespace hmap
{

Path find_cut_path_dijkstra(const Array   &z,
                            DomainBoundary start,
                            DomainBoundary end,
                            float          dijk_elevation_ratio,
                            float          dijk_distance_exponent,
                            float          dijk_upward_penalization)
{
  // --- find lowest point on each boundary

  auto nx = z.shape.x;
  auto ny = z.shape.y;

  auto find_lowest_on_boundary = [&](DomainBoundary b) -> std::pair<int, int>
  {
    int   best_i = -1;
    int   best_j = -1;
    float best_z = std::numeric_limits<float>::infinity();

    switch (b)
    {
    case BOUNDARY_LEFT: // i = 0
      for (int j = 0; j < ny; ++j)
      {
        float v = z(0, j);
        if (v < best_z)
        {
          best_z = v;
          best_i = 0;
          best_j = j;
        }
      }
      break;

    case BOUNDARY_RIGHT: // i = nx - 1
      for (int j = 0; j < ny; ++j)
      {
        float v = z(nx - 1, j);
        if (v < best_z)
        {
          best_z = v;
          best_i = nx - 1;
          best_j = j;
        }
      }
      break;

    case BOUNDARY_BOTTOM: // j = 0
      for (int i = 0; i < nx; ++i)
      {
        float v = z(i, 0);
        if (v < best_z)
        {
          best_z = v;
          best_i = i;
          best_j = 0;
        }
      }
      break;

    case BOUNDARY_TOP: // j = ny - 1
      for (int i = 0; i < nx; ++i)
      {
        float v = z(i, ny - 1);
        if (v < best_z)
        {
          best_z = v;
          best_i = i;
          best_j = ny - 1;
        }
      }
      break;
    }

    return {best_i, best_j};
  };

  auto start_pt = find_lowest_on_boundary(start);
  auto end_pt = find_lowest_on_boundary(end);

  // --- find cut path

  std::vector<int> i_path, j_path;

  find_path_dijkstra(z,
                     Vec2<int>(start_pt.first, start_pt.second),
                     Vec2<int>(end_pt.first, end_pt.second),
                     i_path,
                     j_path,
                     dijk_elevation_ratio,
                     dijk_distance_exponent,
                     dijk_upward_penalization);

  // --- build the output path

  std::vector<float> x, y, v;
  x.reserve(i_path.size());
  y.reserve(i_path.size());
  v.reserve(i_path.size());

  for (size_t k = 0; k < i_path.size(); ++k)
  {
    x.push_back(float(i_path[k]) / float(nx - 1));
    y.push_back(float(j_path[k]) / float(ny - 1));
    v.push_back(z(i_path[k], j_path[k]));
  }

  return Path(x, y, v);
}

Path find_cut_path_midpoint(const Array   &z,
                            DomainBoundary start,
                            DomainBoundary end,
                            uint           seed,
                            int            midp_iterations,
                            float          midp_sigma)
{
    // --- find lowest point on each boundary

  auto nx = z.shape.x;
  auto ny = z.shape.y;

  auto find_lowest_on_boundary = [&](DomainBoundary b) -> std::pair<int, int>
  {
    int   best_i = -1;
    int   best_j = -1;
    float best_z = std::numeric_limits<float>::infinity();

    switch (b)
    {
    case BOUNDARY_LEFT: // i = 0
      for (int j = 0; j < ny; ++j)
      {
        float v = z(0, j);
        if (v < best_z)
        {
          best_z = v;
          best_i = 0;
          best_j = j;
        }
      }
      break;

    case BOUNDARY_RIGHT: // i = nx - 1
      for (int j = 0; j < ny; ++j)
      {
        float v = z(nx - 1, j);
        if (v < best_z)
        {
          best_z = v;
          best_i = nx - 1;
          best_j = j;
        }
      }
      break;

    case BOUNDARY_BOTTOM: // j = 0
      for (int i = 0; i < nx; ++i)
      {
        float v = z(i, 0);
        if (v < best_z)
        {
          best_z = v;
          best_i = i;
          best_j = 0;
        }
      }
      break;

    case BOUNDARY_TOP: // j = ny - 1
      for (int i = 0; i < nx; ++i)
      {
        float v = z(i, ny - 1);
        if (v < best_z)
        {
          best_z = v;
          best_i = i;
          best_j = ny - 1;
        }
      }
      break;
    }

    return {best_i, best_j};
  };

  auto start_pt = find_lowest_on_boundary(start);
  auto end_pt = find_lowest_on_boundary(end);

  // --- find cut path

  std::vector<int> i_path, j_path;
  i_path = {start_pt.first, int(0.5f * (start_pt.first + end_pt.first)), end_pt.first};
  j_path = {start_pt.second, int(0.5f * (start_pt.second + end_pt.second)), end_pt.second};
  
  std::vector<float> x, y, v;

  x.reserve(i_path.size());
  y.reserve(i_path.size());
  v.reserve(i_path.size());

  for (size_t k = 0; k < i_path.size(); ++k)
  {
    x.push_back(float(i_path[k]) / float(nx - 1));
    y.push_back(float(j_path[k]) / float(ny - 1));
    v.push_back(z(i_path[k], j_path[k]));
  }

  auto path = Path(x, y, v);

  path.fractalize(midp_iterations,
		  seed,
		  midp_sigma);

  path.bspline();
  
  return path;
}

} // namespace hmap
