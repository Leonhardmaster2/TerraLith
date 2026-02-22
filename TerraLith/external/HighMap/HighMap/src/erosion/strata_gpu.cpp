/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "macrologger.h"

#include "highmap/array.hpp"
#include "highmap/opencl/gpu_opencl.hpp"

namespace hmap::gpu
{

void strata(Array             &z,
            float              angle,
            float              slope,
            float              gamma,
            uint               seed,
            bool               linear_gamma,
            float              kz,
            int                octaves,
            float              lacunarity,
            float              gamma_noise_ratio,
            float              noise_amp,
            const Vec2<float> &noise_kw,
            const Vec2<float> &ridge_noise_kw,
            float              ridge_angle_shift,
            float              ridge_noise_amp,
            float              ridge_clamp_vmin,
            float              ridge_remap_vmin,
            bool               apply_elevation_mask,
            bool               apply_ridge_mask,
            float              mask_gamma,
            const Array       *p_mask,
            const Vec4<float> &bbox)
{
  auto run = clwrapper::Run("strata");

  run.bind_buffer<float>("z", z.vector);

  helper_bind_optional_buffer(run, "mask", p_mask);

  run.bind_arguments(z.shape.x,
                     z.shape.y,
                     angle,
                     slope,
                     gamma,
                     seed,
                     linear_gamma ? 1 : 0,
                     kz,
                     octaves,
                     lacunarity,
                     gamma_noise_ratio,
                     noise_amp,
                     noise_kw,
                     ridge_noise_kw,
                     ridge_angle_shift,
                     ridge_noise_amp,
                     ridge_clamp_vmin,
                     ridge_remap_vmin,
                     apply_elevation_mask ? 1 : 0,
                     apply_ridge_mask ? 1 : 0,
                     mask_gamma,
                     p_mask ? 1 : 0,
                     bbox);

  run.write_buffer("z");
  run.execute({z.shape.x, z.shape.y});
  run.read_buffer("z");
}

} // namespace hmap::gpu
