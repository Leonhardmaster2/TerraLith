#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::remap(z);

  auto z1 = z;
  hmap::remap(z1, -2.f, 2.f);
  float width = 0.2f;
  z1 = hmap::sigmoid(z1, width);

  hmap::export_banner_png("ex_sigmoid.png", {z, z1}, hmap::Cmap::INFERNO);
}
