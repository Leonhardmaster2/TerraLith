#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  hmap::Array water_depth = hmap::flooding_lake_system(z, 500, 1e-4f);

  float dry_out_ratio = 0.5f;
  auto  w1 = water_depth;
  hmap::water_depth_dry_out(w1, dry_out_ratio);

  // remove water at high elevations
  dry_out_ratio = 1.f;
  auto w2 = water_depth;
  auto mask = 1.f - z;
  hmap::water_depth_dry_out(w2, dry_out_ratio, &mask);

  hmap::export_banner_png("ex_water_depth_dry_out.png",
                          {z, z + water_depth, z + w1, z + w2},
                          hmap::Cmap::TERRAIN,
                          true);
}
