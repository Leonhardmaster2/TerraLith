#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  auto z1 = z;

  float threshold = 0.7f;
  float scaling = 0.5f;
  float transition_extent = 0.2f;

  hmap::reverse_above_theshold(z1, threshold, scaling, transition_extent);

  // array input
  auto z2 = z;

  hmap::Array threshold_array = hmap::noise(hmap::NoiseType::PERLIN,
                                            shape,
                                            res,
                                            ++seed);
  hmap::remap(threshold_array, 0.6f, 0.8f);

  scaling = 0.2f;
  hmap::reverse_above_theshold(z2, threshold_array, scaling, transition_extent);

  z2.dump();

  hmap::export_banner_png("ex_reverse_above_theshold.png",
                          {z, z1, z2},
                          hmap::Cmap::TERRAIN,
                          true);
}
