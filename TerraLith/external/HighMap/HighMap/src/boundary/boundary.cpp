/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <cmath>

#include "macrologger.h"

#include "highmap/array.hpp"
#include "highmap/boundary.hpp"
#include "highmap/math.hpp"
#include "highmap/operator.hpp"
#include "highmap/transform.hpp"

namespace hmap
{

void extrapolate_borders(Array &array, int nbuffer, float sigma)
{
  const int ni = array.shape.x;
  const int nj = array.shape.y;

  if (sigma == 0.f)
  {
    for (int j = 0; j < nj; j++)
      for (int k = nbuffer - 1; k > -1; k--)
      {
        array(k, j) = 2.f * array(k + 1, j) - array(k + 2, j);
        array(ni - 1 - k, j) = 2.f * array(ni - 2 - k, j) -
                               array(ni - 3 - k, j);
      }

    for (int i = 0; i < ni; i++)
      for (int k = nbuffer - 1; k > -1; k--)
      {
        array(i, k) = 2.f * array(i, k + 1) - array(i, k + 2);
        array(i, nj - 1 - k) = 2.f * array(i, nj - 2 - k) -
                               array(i, nj - 3 - k);
      }
  }
  else
  {
    for (int j = 0; j < nj; j++)
    {
      float vref1 = array(nbuffer, j);
      float vref2 = array(ni - 1 - nbuffer, j);

      for (int k = nbuffer - 1; k > -1; k--)
      {
        array(k, j) = 2.f * array(k + 1, j) - array(k + 2, j);
        array(ni - 1 - k, j) = 2.f * array(ni - 2 - k, j) -
                               array(ni - 3 - k, j);

        array(k, j) = (1.f - sigma) * array(k, j) + sigma * vref1;
        array(ni - 1 - k, j) = (1.f - sigma) * array(ni - 1 - k, j) +
                               sigma * vref2;
      }
    }

    for (int i = 0; i < ni; i++)
    {
      float vref1 = array(i, nbuffer);
      float vref2 = array(i, nj - 1 - nbuffer);

      for (int k = nbuffer - 1; k > -1; k--)
      {
        array(i, k) = 2.f * array(i, k + 1) - array(i, k + 2);
        array(i, nj - 1 - k) = 2.f * array(i, nj - 2 - k) -
                               array(i, nj - 3 - k);

        array(i, k) = (1.f - sigma) * array(i, k) + sigma * vref1;
        array(i, nj - 1 - k) = (1.f - sigma) * array(i, nj - 1 - k) +
                               sigma * vref2;
      }
    }
  }
}

void falloff(Array           &array,
             float            strength,
             DistanceFunction dist_fct,
             const Array     *p_noise,
             Vec4<float>      bbox)
{
  hmap::Vec2<float> shift = {bbox.a, bbox.c};
  hmap::Vec2<float> scale = {bbox.b - bbox.a, bbox.d - bbox.c};

  std::vector<float> x = linspace(shift.x - 0.5f,
                                  shift.x - 0.5f + scale.x,
                                  array.shape.x,
                                  false);
  std::vector<float> y = linspace(shift.y - 0.5f,
                                  shift.y - 0.5f + scale.y,
                                  array.shape.y,
                                  false);

  std::function<float(float, float)> r_fct = get_distance_function(dist_fct);

  if (!p_noise)
    for (int j = 0; j < array.shape.y; j++)
      for (int i = 0; i < array.shape.x; i++)
      {
        float r = r_fct(2.f * x[i], 2.f * y[j]);
        r = 1.f - strength * r * r;
        array(i, j) *= r;
      }
  else
    for (int j = 0; j < array.shape.y; j++)
      for (int i = 0; i < array.shape.x; i++)
      {
        float r = r_fct(x[i], y[j]);
        r += (*p_noise)(i, j) * (*p_noise)(i, j);
        r = 1.f - strength * r * r;
        array(i, j) *= r;
      }
}

void fill_borders(Array &array)
{
  const int ni = array.shape.x;
  const int nj = array.shape.y;

  for (int j = 0; j < nj; j++)
  {
    array(0, j) = array(1, j);
    array(ni - 1, j) = array(ni - 2, j);
  }

  for (int i = 0; i < ni; i++)
  {
    array(i, 0) = array(i, 1);
    array(i, nj - 1) = array(i, nj - 2);
  }
}

void fill_borders(Array &array, int nbuffer)
{
  const int ni = array.shape.x;
  const int nj = array.shape.y;

  for (int j = 0; j < nj; j++)
    for (int i = nbuffer - 1; i >= 0; i--)
    {
      array(i, j) = array(i + 1, j);
      array(ni - i - 1, j) = array(ni - i - 2, j);
    }

  for (int j = nbuffer - 1; j >= 0; j--)
    for (int i = 0; i < ni; i++)
    {
      array(i, j) = array(i, j + 1);
      array(i, nj - j - 1) = array(i, nj - j - 2);
    }
}

Array generate_buffered_array(const Array &array,
                              Vec4<int>    buffers,
                              bool         zero_padding)
{
  Array array_out = Array(Vec2<int>(array.shape.x + buffers.a + buffers.b,
                                    array.shape.y + buffers.c + buffers.d));

  for (int j = 0; j < array.shape.y; j++)
    for (int i = 0; i < array.shape.x; i++)
      array_out(i + buffers.a, j + buffers.c) = array(i, j);

  if (!zero_padding)
  {
    int i1 = buffers.a;
    int i2 = buffers.b;
    int j1 = buffers.c;
    int j2 = buffers.d;

    for (int j = j1; j < array_out.shape.y - j2; j++)
      for (int i = 0; i < i1; i++)
        array_out(i, j) = array_out(2 * i1 - i, j);

    for (int j = j1; j < array_out.shape.y - j2; j++)
      for (int i = array_out.shape.x - i2; i < array_out.shape.x; i++)
        array_out(i, j) = array_out(2 * (array_out.shape.x - i2) - i - 1, j);

    for (int j = 0; j < j1; j++)
      for (int i = 0; i < array_out.shape.x; i++)
        array_out(i, j) = array_out(i, 2 * j1 - j);

    for (int j = array_out.shape.y - j2; j < array_out.shape.y; j++)
      for (int i = 0; i < array_out.shape.x; i++)
        array_out(i, j) = array_out(i, 2 * (array_out.shape.y - j2) - j - 1);
  }

  return array_out;
}

void make_periodic(Array                 &array,
                   int                    nbuffer,
                   const PeriodicityType &periodicity_type)
{
  const int ni = array.shape.x;
  const int nj = array.shape.y;

  if (nbuffer <= 0) return;
  if (nbuffer > ni / 2 || nbuffer > nj / 2) nbuffer = std::min(ni, nj) / 2;

  auto compute_weight = [&](int k) -> float
  {
    // k in [0, nbuffer-1]
    if (nbuffer <= 1) return 0.0f;

    float t = 0.5f * (float)k / (float)(nbuffer - 1);
    return 0.5f * smoothstep3(2.f * t);
  };

  // blends a pair of values symmetrically:
  //    out0 = (0.5 + r)*a0 + (0.5 - r)*a1
  //    out1 = (0.5 + r)*a1 + (0.5 - r)*a0
  auto blend_pair = [&](float &v0, float &v1, float r)
  {
    const float w0 = 0.5f + r;
    const float w1 = 0.5f - r;
    const float a0 = v0;
    const float a1 = v1;
    v0 = w0 * a0 + w1 * a1;
    v1 = w0 * a1 + w1 * a0;
  };

  Array tmp = array; // X pass writes to tmp, Y pass reads from tmp

  // --- X periodicity

  if (periodicity_type == PeriodicityType::PERIODICITY_X ||
      periodicity_type == PeriodicityType::PERIODICITY_XY)
  {
    for (int i = 0; i < nbuffer; ++i)
    {
      float r = compute_weight(i);
      int   ir = ni - 1 - i;

      for (int j = 0; j < nj; ++j)
        blend_pair(tmp(i, j), tmp(ir, j), r);
    }
  }

  Array result = tmp;

  // --- Y periodicity

  if (periodicity_type == PeriodicityType::PERIODICITY_Y ||
      periodicity_type == PeriodicityType::PERIODICITY_XY)
  {
    for (int j = 0; j < nbuffer; ++j)
    {
      float r = compute_weight(j);
      int   jr = nj - 1 - j;

      for (int i = 0; i < ni; ++i)
        blend_pair(result(i, j), result(i, jr), r);
    }
  }

  array = result;
}

Array make_periodic_stitching(const Array &array, float overlap)
{
  Array     array_p = array;
  Vec2<int> shape = array.shape;

  Vec2<int> noverlap = {(int)(0.5f * overlap * shape.x),
                        (int)(0.5f * overlap * shape.y)};
  int       ir = (int)noverlap.x / 2.f;

  // east frontier
  {
    Array error = Array(Vec2<int>(noverlap.x, shape.y));
    for (int j = 0; j < shape.y; j++)
      for (int i = 0; i < noverlap.x; i++)
        error(i, j) = std::abs(array(i, j) -
                               array(shape.x - 1 - noverlap.x + i, j));

    // find cut path
    std::vector<int> cut_path_i;
    find_vertical_cut_path(error, cut_path_i);

    // define lerp factor
    Array mask = generate_mask(error.shape, cut_path_i, ir);

    for (int j = 0; j < shape.y; j++)
      for (int i = 0; i < noverlap.x; i++)
        array_p(i, j) = lerp(array(shape.x - 1 - noverlap.x + i, j),
                             array(i, j),
                             mask(i, j));
  }

  // south frontier
  {
    Array error = Array(Vec2<int>(shape.x, noverlap.y));
    for (int j = 0; j < noverlap.y; j++)
      for (int i = 0; i < shape.x; i++)
        error(i, j) = std::abs(array_p(i, j) -
                               array_p(i, shape.y - 1 - noverlap.y + j));

    Array mask = Array(error.shape);
    {
      Array            error_t = transpose(error);
      std::vector<int> cut_path_i;
      find_vertical_cut_path(error_t, cut_path_i);
      Array mask_t = generate_mask(error_t.shape, cut_path_i, ir);
      mask = transpose(mask_t);
    }

    for (int j = 0; j < noverlap.y; j++)
      for (int i = 0; i < shape.x; i++)
        array_p(i, j) = lerp(array_p(i, shape.y - 1 - noverlap.y + j),
                             array_p(i, j),
                             mask(i, j));
  }

  int nx = (int)(0.5f * noverlap.x);
  int ny = (int)(0.5f * noverlap.y);

  array_p = array_p.extract_slice(Vec4<int>(nx,
                                            array.shape.x - noverlap.x + nx,
                                            ny,
                                            array.shape.y - noverlap.y + ny));

  array_p = array_p.resample_to_shape(shape);

  return array_p;
}

Array make_periodic_tiling(const Array &array, float overlap, Vec2<int> tiling)
{
  //

  Array array_periodic = make_periodic_stitching(array, overlap);

  Vec2<int> shape_tile = {array.shape.x / tiling.x, array.shape.y / tiling.y};
  array_periodic = array_periodic.resample_to_shape(shape_tile);

  Array array_out = array_periodic;

  for (int k = 1; k < tiling.x; k++)
    array_out = hstack(array_out, array_periodic);

  Array array_strip = array_out;

  for (int k = 1; k < tiling.y; k++)
    array_out = vstack(array_out, array_strip);

  if (array_out.shape != array.shape)
    array_out = array_out.resample_to_shape(array.shape);

  return array_out;
}

void set_borders(Array      &array,
                 Vec4<float> border_values,
                 Vec4<int>   buffer_sizes)
{
  // west
  for (int j = 0; j < array.shape.y; j++)
    for (int i = 0; i < buffer_sizes.a; i++)
    {
      float r = (float)i / (float)buffer_sizes.a;
      r = r * r * (3.f - 2.f * r);
      array(i, j) = (1.f - r) * border_values.a + r * array(i, j);
    }

  // east
  for (int j = 0; j < array.shape.y; j++)
    for (int i = array.shape.x - buffer_sizes.b; i < array.shape.x; i++)
    {
      float r = 1.f - (float)(i - array.shape.x + buffer_sizes.b) /
                          (float)buffer_sizes.b;
      r = r * r * (3.f - 2.f * r);
      array(i, j) = (1.f - r) * border_values.b + r * array(i, j);
    }

  // south
  for (int j = 0; j < buffer_sizes.c; j++)
    for (int i = 0; i < array.shape.x; i++)
    {
      float r = (float)j / (float)buffer_sizes.c;
      r = r * r * (3.f - 2.f * r);
      array(i, j) = (1.f - r) * border_values.c + r * array(i, j);
    }

  // north
  for (int j = array.shape.y - buffer_sizes.d; j < array.shape.y; j++)
    for (int i = 0; i < array.shape.x; i++)
    {
      float r = 1.f - (float)(j - array.shape.y + buffer_sizes.d) /
                          (float)buffer_sizes.d;
      r = r * r * (3.f - 2.f * r);
      array(i, j) = (1.f - r) * border_values.d + r * array(i, j);
    }
}

void set_borders(Array &array, float border_values, int buffer_sizes)
{
  Vec4<float> bv = Vec4<float>(border_values,
                               border_values,
                               border_values,
                               border_values);
  Vec4<int>   bs = Vec4<int>(buffer_sizes,
                           buffer_sizes,
                           buffer_sizes,
                           buffer_sizes);
  set_borders(array, bv, bs);
}

void sym_borders(Array &array, Vec4<int> buffer_sizes)
{
  const int i1 = buffer_sizes.a;
  const int i2 = buffer_sizes.b;
  const int j1 = buffer_sizes.c;
  const int j2 = buffer_sizes.d;

  // fill-in the blanks...
  for (int j = j1; j < array.shape.y - j2; j++)
    for (int i = 0; i < i1; i++)
      array(i, j) = array(2 * i1 - i, j);

  for (int j = j1; j < array.shape.y - j2; j++)
    for (int i = array.shape.x - i2; i < array.shape.x; i++)
      array(i, j) = array(2 * (array.shape.x - i2) - i - 1, j);

  for (int j = 0; j < j1; j++)
    for (int i = 0; i < array.shape.x; i++)
      array(i, j) = array(i, 2 * j1 - j);

  for (int j = array.shape.y - j2; j < array.shape.y; j++)
    for (int i = 0; i < array.shape.x; i++)
      array(i, j) = array(i, 2 * (array.shape.y - j2) - j - 1);
}

void zeroed_borders(Array &array)
{
  const int ni = array.shape.x;
  const int nj = array.shape.y;

  for (int j = 0; j < nj; j++)
  {
    array(0, j) = 0.f;
    array(ni - 1, j) = 0.f;
  }

  for (int i = 0; i < ni; i++)
  {
    array(i, 0) = 0.f;
    array(i, nj - 1) = 0.f;
  }
}

void zeroed_edges(Array           &array,
                  float            sigma,
                  DistanceFunction dist_fct,
                  const Array     *p_noise,
                  Vec4<float>      bbox)
{
  hmap::Vec2<float> shift = {bbox.a, bbox.c};
  hmap::Vec2<float> scale = {bbox.b - bbox.a, bbox.d - bbox.c};

  std::vector<float> x = linspace(shift.x - 0.5f,
                                  shift.x - 0.5f + scale.x,
                                  array.shape.x,
                                  false);
  std::vector<float> y = linspace(shift.y - 0.5f,
                                  shift.y - 0.5f + scale.y,
                                  array.shape.y,
                                  false);

  std::function<float(float, float)> r_fct = get_distance_function(dist_fct);

  if (!p_noise)
    for (int j = 0; j < array.shape.y; j++)
      for (int i = 0; i < array.shape.x; i++)
      {
        float r = r_fct(2.f * x[i], 2.f * y[j]);
        float ra = r < 1.f ? std::pow(1.f - r, sigma) : 0.f;
        array(i, j) *= ra / (ra + std::pow(r, sigma));
      }
  else
    for (int j = 0; j < array.shape.y; j++)
      for (int i = 0; i < array.shape.x; i++)
      {
        float r = r_fct(2.f * x[i], 2.f * y[j]);
        r += (*p_noise)(i, j) * (*p_noise)(i, j);
        float ra = r < 1.f ? std::pow(1.f - r, sigma) : 0.f;
        array(i, j) *= ra / (ra + std::pow(1.f, sigma));
      }
}

} // namespace hmap
