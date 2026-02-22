/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <cmath>
#include <vector>

#include "macrologger.h"

#include "highmap/array.hpp"
#include "highmap/erosion.hpp"
#include "highmap/functions.hpp"
#include "highmap/geometry/grids.hpp"
#include "highmap/math.hpp"
#include "highmap/operator.hpp"
#include "highmap/primitives.hpp"

namespace hmap
{

Array cone(Vec2<int>    shape,
           float        slope,
           float        apex_elevation,
           bool         smooth_profile,
           Vec2<float>  center,
           const Array *p_noise_x,
           const Array *p_noise_y,
           Vec4<float>  bbox)
{
  Array array = Array(shape);

  auto lambda = [slope, apex_elevation, center](float x, float y, float)
  {
    float dx = x - center.x;
    float dy = y - center.y;
    float r = std::hypot(dx, dy);
    float v = std::max(0.f, apex_elevation - slope * r);
    return v;
  };

  auto lambda_s = [slope, apex_elevation, center](float x, float y, float)
  {
    float dx = x - center.x;
    float dy = y - center.y;
    float r = std::hypot(dx, dy);
    float v = std::max(0.f, apex_elevation - slope * r);
    return almost_unit_identity(v);
  };

  if (smooth_profile)
  {
    fill_array_using_xy_function(array,
                                 bbox,
                                 nullptr,
                                 p_noise_x,
                                 p_noise_y,
                                 nullptr,
                                 lambda_s);
  }
  else
  {
    fill_array_using_xy_function(array,
                                 bbox,
                                 nullptr,
                                 p_noise_x,
                                 p_noise_y,
                                 nullptr,
                                 lambda);
  }

  return array;
}

Array cone_complex(Vec2<int>             shape,
                   float                 alpha,
                   float                 radius,
                   bool                  smooth_profile,
                   float                 valley_amp,
                   int                   valley_nb,
                   float                 valley_decay_ratio,
                   float                 valley_angle0,
                   const ErosionProfile &erosion_profile,
                   float                 erosion_delta,
                   float                 radial_waviness_amp,
                   float                 radial_waviness_kw,
                   float                 bias_angle,
                   float                 bias_amp,
                   float                 bias_exponent,
                   Vec2<float>           center,
                   const Array          *p_ctrl_param,
                   const Array          *p_noise_x,
                   const Array          *p_noise_y,
                   Vec4<float>           bbox)
{
  Array array = Array(shape);

  float valley_alpha0 = valley_angle0 / 180.f * M_PI;
  float dv = 1.f / (valley_decay_ratio * valley_decay_ratio * radius * radius);
  float bias_alpha = bias_angle / 180.f * M_PI;
  float profile_avg; // not used

  auto erosion_profile_fct = get_erosion_profile_function(erosion_profile,
                                                          erosion_delta,
                                                          profile_avg);

  auto lambda = [=](float x, float y, float ctrl)
  {
    float dx = x - center.x;
    float dy = y - center.y;
    float r = std::hypot(dx, dy) / radius;

    if (r > 1.f) return 0.f;

    float theta = std::atan2(dy, dx) + M_PI;
    theta += radial_waviness_amp *
             std::sin(radial_waviness_kw * 2.f * M_PI * r);

    // base shape
    float v = (1.f - std::pow(r, alpha)) / (1.f + std::pow(r, alpha));

    // directional bias
    {
      float b_shape = std::cos(theta - bias_alpha);
      float b_amp = std::pow(r * (1.f - r), bias_exponent);
      v += bias_amp * b_shape * b_amp;
    }

    // valleys
    {
      float v_shape = erosion_profile_fct(float(valley_nb) * theta -
                                          valley_alpha0);
      float v_amp = 1.f - std::exp(-0.5f * r * r * dv);
      v -= ctrl * valley_amp * std::max(0.f, v_shape * v_amp);
    }

    v = std::max(v, 0.f);

    if (smooth_profile)
      return almost_unit_identity(v);
    else
      return v;
  };

  fill_array_using_xy_function(array,
                               bbox,
                               p_ctrl_param,
                               p_noise_x,
                               p_noise_y,
                               nullptr,
                               lambda);

  return array;
}

Array cone_sigmoid(Vec2<int>    shape,
                   float        alpha,
                   float        radius,
                   Vec2<float>  center,
                   const Array *p_noise_x,
                   const Array *p_noise_y,
                   Vec4<float>  bbox)
{
  Array array = Array(shape);

  auto lambda = [alpha, radius, center](float x, float y, float)
  {
    float dx = x - center.x;
    float dy = y - center.y;
    float r = std::hypot(dx, dy) / radius;
    float v = (1.f - std::pow(r, alpha)) / (1.f + std::pow(r, alpha));
    return std::max(0.f, v);
  };

  fill_array_using_xy_function(array,
                               bbox,
                               nullptr,
                               p_noise_x,
                               p_noise_y,
                               nullptr,
                               lambda);

  return array;
}

} // namespace hmap
