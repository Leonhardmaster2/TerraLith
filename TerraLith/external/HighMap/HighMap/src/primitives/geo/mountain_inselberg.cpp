/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "highmap/array.hpp"
#include "highmap/filters.hpp"
#include "highmap/opencl/gpu_opencl.hpp"
#include "highmap/primitives.hpp"
#include "highmap/range.hpp"

namespace hmap::gpu
{

Array mountain_inselberg(Vec2<int>    shape,
                         uint         seed,
                         float        scale,
                         int          octaves,
                         float        rugosity,
                         float        angle,
                         float        gamma,
                         bool         round_shape,
                         bool         add_deposition,
                         float        bulk_amp,
                         float        base_noise_amp,
                         float        k_smoothing,
                         Vec2<float>  center,
                         const Array *p_noise_x,
                         const Array *p_noise_y,
                         Vec4<float>  bbox)
{
  // apply global scaling to reference values
  const float       half_width = 0.2f * scale;
  const Vec2<float> kw = Vec2<float>(2.6f / scale, 2.6f / scale);

  const float persistence = 0.5f;
  const float lacunarity = 2.f;
  const float alpha = angle / 180.f * M_PI;

  // prepare base noise used for displacements
  Array noise = scale * base_noise_amp *
                gpu::noise_fbm(NoiseType::SIMPLEX2,
                               shape,
                               kw,
                               seed,
                               octaves,
                               rugosity,
                               persistence,
                               lacunarity,
                               nullptr,
                               p_noise_x,
                               p_noise_y,
                               nullptr,
                               bbox);

  Array dx = noise * std::cos(alpha);
  Array dy = noise * std::sin(alpha);

  // enveloppe pulse
  Array *p_gx = round_shape ? nullptr : &dx;
  Array *p_gy = round_shape ? nullptr : &dy;

  Array pulse = gaussian_pulse(shape,
                               half_width,
                               /* p_ctrl_param */ nullptr,
                               p_gx,
                               p_gy,
                               /* p_stretching */ nullptr,
                               center,
                               bbox);

  // base primitives
  Vec2<float>       jitter(1.f, 1.f);
  VoronoiReturnType return_type = VoronoiReturnType::CONSTANT_F2MF1_SQUARED;

  Array voronoi = 0.72f + voronoi_fbm(shape,
                                      kw,
                                      seed,
                                      jitter,
                                      k_smoothing,
                                      0.f,
                                      return_type,
                                      octaves,
                                      /* weight */ 0.7f,
                                      persistence,
                                      lacunarity,
                                      nullptr,
                                      &dx,
                                      &dy,
                                      bbox);

  clamp_min(voronoi, 0.f);

  voronoi *= pulse;

  if (bulk_amp > 0.f)
  {
    voronoi += bulk_amp * pulse;
    voronoi *= 1.f / (1.f + bulk_amp);
  }

  gamma_correction(voronoi, gamma);

  if (add_deposition)
  {
    int   ir = (int)(0.05f * scale * shape.x);
    float k = 0.05f;
    gpu::smooth_fill(voronoi, ir, k);
  }

  return voronoi;
}

} // namespace hmap::gpu
