#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);
  auto z0 = z;

  hmap::Array water_depth = hmap::flooding_uniform_level(z, 0.3f);

  float additional_depth = 0.075f;
  int   iterations = 10;

  hmap::coastal_erosion_diffusion(z, water_depth, additional_depth, iterations);

  hmap::export_banner_png("ex_coastal_erosion_diffusion.png",
                          {z0, z, z + water_depth},
                          hmap::Cmap::TERRAIN,
                          true);
}
