#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {2.f, 2.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  float advection_length = 0.05f;
  float value_persistence = 0.96f;

  hmap::Array za = hmap::gpu::advection_warp(z,
                                             z,
                                             advection_length,
                                             value_persistence);

  // advect another field based on the elevation
  hmap::Array n = hmap::noise_fbm(hmap::NoiseType::PERLIN,
                                  shape,
                                  {32.f, 32.f},
                                  seed);

  hmap::Array zb = hmap::gpu::advection_warp(z,
                                             n,
                                             advection_length,
                                             value_persistence);

  hmap::export_banner_png("ex_advection_warp.png",
                          {z, za, zb},
                          hmap::Cmap::TERRAIN,
                          true);
}
