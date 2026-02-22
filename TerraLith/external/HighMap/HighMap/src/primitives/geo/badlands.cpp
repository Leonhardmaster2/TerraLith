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

Array badlands(Vec2<int>    shape,
               Vec2<float>  kw,
               uint         seed,
               int          octaves,
               float        rugosity,
               float        angle,
               float        k_smoothing,
               float        base_noise_amp,
               const Array *p_noise_x,
               const Array *p_noise_y,
               Vec4<float>  bbox)
{
  const float persistence = 0.5f;
  const float lacunarity = 2.3f;
  const float alpha = angle / 180.f * M_PI;

  // prepare base noise used for displacements
  Array noise = base_noise_amp * gpu::noise_fbm(NoiseType::SIMPLEX2,
                                                shape,
                                                kw,
                                                seed++,
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

  // base primitive
  Vec2<float>       jitter(1.f, 1.f);
  VoronoiReturnType return_type = VoronoiReturnType::CONSTANT_F2MF1_SQUARED;

  Array voronoi = voronoi_fbm(shape,
                              kw,
                              seed++,
                              jitter,
                              k_smoothing,
                              0.f,
                              return_type,
                              octaves,
                              /* weight */ 0.5f,
                              persistence,
                              lacunarity,
                              nullptr,
                              &dx,
                              &dy,
                              bbox);

  return voronoi;
}

} // namespace hmap::gpu
