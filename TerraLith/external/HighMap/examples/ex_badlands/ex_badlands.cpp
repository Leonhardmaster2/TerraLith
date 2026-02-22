#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {2.f, 2.f};
  uint              seed = 0;

  hmap::Array z1 = hmap::gpu::badlands(shape, kw, seed);

  hmap::export_banner_png("ex_badlands.png", {z1}, hmap::Cmap::TERRAIN, true);
}
