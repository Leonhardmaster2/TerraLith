#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {2.f, 2.f};
  int               seed = 1;

  int nparticles = 10000;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  hmap::Array za = z;
  za = hmap::gpu::advection_particle(z, za, nparticles, seed);

  // advect another field based on the elevation
  hmap::Array n0 = hmap::noise_fbm(hmap::NoiseType::PERLIN,
                                   shape,
                                   {32.f, 32.f},
                                   seed);

  hmap::Array n = n0;

  int iterations = 10;
  n = hmap::gpu::advection_particle(z, n0, iterations, nparticles, seed);

  hmap::export_banner_png("ex_advection_particle.png",
                          {z, za, n},
                          hmap::Cmap::TERRAIN,
                          true);
}
