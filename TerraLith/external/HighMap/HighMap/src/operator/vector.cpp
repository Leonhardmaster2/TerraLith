/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

#include <algorithm>
#include <list>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "macrologger.h"

namespace hmap
{

std::vector<float> gradient1d(const std::vector<float> &v)
{
  size_t             n = v.size();
  std::vector<float> dv(n);

  if (n > 1)
  {
    for (size_t i = 1; i < n; i++)
      dv[i] = 0.5f * (v[i + 1] - v[i - 1]);
    dv[0] = v[1] - v[0];
    dv[n - 1] = v[n - 1] - v[n - 2];
  }
  return dv;
}

void laplace1d(std::vector<float> &v, float sigma = 0.5f, int iterations = 1)
{
  size_t             n = v.size();
  std::vector<float> d(n);

  for (int it = 0; it < iterations; it++)
  {
    for (size_t i = 1; i < n - 1; i++)
      d[i] = 2.f * v[i] - v[i - 1] - v[i + 1];

    for (size_t i = 1; i < n - 1; i++)
      v[i] -= sigma * d[i];
  }
}

std::vector<float> linspace(float start,
                            float stop,
                            int   num,
                            bool  endpoint = true)
{
  if (num < 0) throw std::invalid_argument("num must be non-negative");

  std::vector<float> v(num);
  if (num == 0) return v;

  if (num == 1)
  {
    v[0] = start;
    return v;
  }

  float dv = endpoint ? (stop - start) / static_cast<float>(num - 1)
                      : (stop - start) / static_cast<float>(num);

  for (int i = 0; i < num; ++i)
    v[i] = start + i * dv;

  return v;
}

std::vector<float> linspace_jitted(float start,
                                   float stop,
                                   int   num,
                                   float ratio,
                                   int   seed,
                                   bool  endpoint)
{
  if (num < 0) throw std::invalid_argument("num must be non-negative");

  std::vector<float> v(num);
  if (num == 0) return v;

  if (num == 1)
  {
    v[0] = start;
    return v;
  }

  float dv = endpoint ? (stop - start) / static_cast<float>(num - 1)
                      : (stop - start) / static_cast<float>(num);

  std::mt19937                          gen(seed);
  std::uniform_real_distribution<float> dis(-0.5f, 0.5f);

  for (int i = 0; i < num; ++i)
  {
    v[i] = start + i * dv;

    // jitter all but the first and last point (to keep range stable)
    if (i > 0 && (!endpoint || i < num - 1)) v[i] += ratio * dis(gen) * dv;
  }

  return v;
}

std::vector<float> random_vector(float min, float max, int num, int seed)
{
  std::vector<float>                    v(num);
  std::mt19937                          gen(seed);
  std::uniform_real_distribution<float> dis(min, max);

  for (auto &v : v)
    v = dis(gen);
  return v;
}

void rescale_vector(std::vector<float> &vec, float vmin, float vmax)
{
  if (vec.empty()) return;

  // fringe case: flatten all values if target range is degenerate
  if (vmin == vmax)
  {
    for (auto &v : vec)
      v = vmax;
    return;
  }

  // current min and max
  float cmin = *std::min_element(vec.begin(), vec.end());
  float cmax = *std::max_element(vec.begin(), vec.end());

  // fringe case: constant input
  if (cmin == cmax)
  {
    for (auto &v : vec)
      v = cmax;
    return;
  }

  // standard case: normalize to [0,1], then scale to [vmin,vmax]
  for (auto &v : vec)
  {
    v = (v - cmin) / (cmax - cmin); // normalize
    v = vmin + (vmax - vmin) * v;   // scale
  }
}

std::vector<float> rescaled_vector(const std::vector<float> &vec,
                                   float                     vmin,
                                   float                     vmax)
{
  std::vector<float> out = vec;
  rescale_vector(out, vmin, vmax);
  return out;
}

} // namespace hmap
