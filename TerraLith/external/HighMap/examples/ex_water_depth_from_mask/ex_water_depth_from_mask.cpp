#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  hmap::Array mask = hmap::select_rivers(z, 1.f / shape.x, 50.f);
  hmap::remap(mask);

  float       mask_threshold = 0.1f; // higher values decrease water extent
  hmap::Array water_depth = hmap::water_depth_from_mask(z,
                                                        mask,
                                                        mask_threshold);

  hmap::export_banner_png("ex_water_depth_from_mask.png",
                          {z, mask, water_depth, z + water_depth},
                          hmap::Cmap::JET);
}
