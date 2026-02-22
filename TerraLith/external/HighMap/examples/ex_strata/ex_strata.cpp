#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  shape = {1024, 1024};
  hmap::Vec2<float> kw = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z1 = 1.f +
                   hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, kw, seed);
  hmap::zeroed_edges(z1);
  hmap::remap(z1);

  hmap::Array z2 = z1;
  hmap::gpu::strata(z2, 30.f, 2.f, 0.7f, ++seed);
  hmap::remap(z2);

  z1.dump("out0.png");
  z2.dump();

  hmap::export_banner_png("ex_strata.png", {z1, z2}, hmap::Cmap::TERRAIN, true);
}
