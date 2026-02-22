#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  hmap::Array water_depth = hmap::flooding_uniform_level(z, 0.3f);
  // hmap::Array water_depth = hmap::flooding_lake_system(z, 500, 1e-4f);

  hmap::Array mask0 = hmap::water_mask(water_depth);
  hmap::Array mask1 = hmap::water_mask(water_depth, z, 0.075f);

  hmap::export_banner_png("ex_water_mask.png",
                          {z, z + water_depth, mask0, mask1},
                          hmap::Cmap::MAGMA);
}
