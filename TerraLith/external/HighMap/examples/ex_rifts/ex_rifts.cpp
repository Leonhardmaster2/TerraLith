#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {4.f, 4.f};
  int               seed = 0;

  hmap::Array z1 = 1.f +
                   hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, kw, seed);
  hmap::zeroed_edges(z1);
  hmap::remap(z1);

  hmap::Array z2 = z1;

  hmap::Vec2<float> kw_r = {8.f, 1.5f};
  float             angle = 30.f;
  float             intensity = 0.1f;

  hmap::gpu::rifts(z2, kw_r, angle, intensity, ++seed);
  hmap::remap(z2);

  hmap::export_banner_png("ex_rifts.png", {z1, z2}, hmap::Cmap::TERRAIN, true);
}
