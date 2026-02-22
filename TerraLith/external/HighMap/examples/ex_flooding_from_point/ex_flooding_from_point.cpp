#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  int i, j;

  i = 32;
  j = 64;
  hmap::Array water_depth1 = hmap::flooding_from_point(z, i, j);

  i = 64;
  j = 150;
  float       depth_min = 0.3f;
  hmap::Array water_depth2 = hmap::flooding_from_point(z, i, j, depth_min);

  hmap::Array water_depth3 = hmap::flooding_from_point(z, {32, 64}, {64, 150});

  float       water_zmax = 0.35f;
  hmap::Array water_depth4 = flooding_from_boundaries(z, water_zmax);

  hmap::Array water_depth5 = flooding_uniform_level(z, water_zmax);

  hmap::export_banner_png("ex_flooding_from_point.png",
                          {z,
                           water_depth1,
                           z + water_depth1,
                           water_depth2,
                           z + water_depth2,
                           water_depth3,
                           z + water_depth3,
                           water_depth4,
                           z + water_depth4,
                           water_depth5,
                           z + water_depth5},
                          hmap::Cmap::JET);
}
