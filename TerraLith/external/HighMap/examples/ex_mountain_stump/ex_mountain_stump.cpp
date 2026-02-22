#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  shape = {1024, 1024};
  uint seed = 3;

  hmap::Array z1 = hmap::gpu::mountain_stump(shape, seed);

  z1.dump();

  hmap::export_banner_png("ex_mountain_stump.png",
                          {z1},
                          hmap::Cmap::TERRAIN,
                          true);
}
