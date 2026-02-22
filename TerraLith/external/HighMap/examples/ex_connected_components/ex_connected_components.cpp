#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  uint              seed = 5;

  hmap::Array z = hmap::noise(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::clamp_min(z, 0.f);

  hmap::Array labels = hmap::connected_components(z);

  std::vector<float>                surfaces;
  std::vector<std::array<float, 2>> centroids;
  auto dummy = hmap::connected_components(z, 0.f, 0.f, &surfaces, &centroids);

  // centroids of each components in index (i, j) scale
  for (auto &v : centroids)
    LOG_DEBUG("%f %f", v[0], v[1]);

  z.to_png("ex_connected_components0.png", hmap::Cmap::INFERNO);
  labels.to_png("ex_connected_components1.png", hmap::Cmap::NIPY_SPECTRAL);
}
