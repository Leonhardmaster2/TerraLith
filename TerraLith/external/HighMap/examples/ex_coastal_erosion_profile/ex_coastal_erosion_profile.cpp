#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {2.f, 2.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::SIMPLEX2S, shape, res, seed);
  hmap::remap(z);
  auto z0 = z;

  hmap::Array water_depth = hmap::flooding_uniform_level(z, 0.3f);

  float shore_ground_extent = 32.f; // pixels
  float shore_water_extent = 16.f;

  hmap::coastal_erosion_profile(z,
                                water_depth,
                                shore_ground_extent,
                                shore_water_extent);

  hmap::export_banner_png("ex_coastal_erosion_profile.png",
                          {z0, z, z + water_depth},
                          hmap::Cmap::TERRAIN,
                          true);
}
