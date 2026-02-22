#include <iostream>

#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);

  hmap::Vec2<int> export_shape = {64, 64};
  std::string     str = hmap::export_as_ascii(z, export_shape);

  std::cout << str << "\n";
}
