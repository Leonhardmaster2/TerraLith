#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z0 = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z0);

  auto mask = z0;
  hmap::clamp_min(mask, 0.5f);
  hmap::remap(mask);

  hmap::Array z1 = z0;
  hmap::remap(z1, 0.f, 0.5f);

  int         iterations = 500;
  hmap::Array z2 = hmap::diffusion_retargeting(z0, z1, mask, iterations);

  hmap::export_banner_png("ex_diffusion_retargeting.png",
                          {z0, z1, z2, mask},
                          hmap::Cmap::TERRAIN);
}
