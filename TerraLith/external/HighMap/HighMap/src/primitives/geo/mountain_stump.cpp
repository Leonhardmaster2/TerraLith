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

Array mountain_stump(Vec2<int>    shape,
                     uint         seed,
                     float        scale,
                     int          octaves,
                     float        peak_kw,
                     float        rugosity,
                     float        angle,
                     float        k_smoothing,
                     float        gamma,
                     bool         add_deposition,
                     float        ridge_amp,
                     float        base_noise_amp,
                     Vec2<float>  center,
                     const Array *p_noise_x,
                     const Array *p_noise_y,
                     Vec4<float>  bbox)
{
  // apply global scaling to reference values
  const float       half_width = 0.1f * scale;
  const Vec2<float> kw = Vec2<float>(peak_kw / scale, peak_kw / scale);

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
                               /* p_ctrl_param */ nullptr,
                               p_noise_x,
                               p_noise_y,
                               /* p_stretching */ nullptr,
                               bbox);

  Array dx = noise * std::cos(alpha);
  Array dy = noise * std::sin(alpha);

  // enveloppe pulse
  Array pulse = gaussian_pulse(shape,
                               half_width,
                               /* p_ctrl_param */ nullptr,
                               /* p_noise_x */ nullptr,
                               /* p_noise_y */ nullptr,
                               /* p_stretching */ nullptr,
                               center,
                               bbox);
  gain(pulse, 2.f);

  // in [0.5, 1]
  Array stump = 0.25f * gpu::noise_fbm(NoiseType::SIMPLEX2,
                                       shape,
                                       kw,
                                       seed,
                                       octaves,
                                       /* rugosity */ 0.7f,
                                       persistence,
                                       lacunarity,
                                       /* p_ctrl_param */ nullptr,
                                       p_noise_x,
                                       p_noise_y,
                                       /* p_stretching */ nullptr,
                                       bbox) +
                0.75f;

  // divide by 0.75f to set amplitude back to [0, 1] (very
  // approximative...)
  const float km = 0.05f;
  stump = hmap::minimum_smooth(stump, pulse, km); // / 0.75f;

  stump.infos();

  // base primitives
  Vec2<float>       jitter(1.f, 1.f);
  VoronoiReturnType return_type = VoronoiReturnType::EDGE_DISTANCE_SQUARED;

  // roughly in [0, 1]
  Array voronoi = 2.f * voronoi_fbm(shape,
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
                                    /* p_ctrl_param */ nullptr,
                                    &dx,
                                    &dy,
                                    bbox);
  clamp_min(voronoi, 0.f);
  voronoi *= pulse;
  gamma_correction(voronoi, gamma);

  voronoi = (ridge_amp * voronoi + stump) / (ridge_amp + 1.f);

  if (add_deposition)
  {
    int   ir = (int)(0.05f * scale * shape.x);
    float k = 0.05f;
    gpu::smooth_fill(voronoi, ir, k);
  }

  return voronoi;
}

} // namespace hmap::gpu
