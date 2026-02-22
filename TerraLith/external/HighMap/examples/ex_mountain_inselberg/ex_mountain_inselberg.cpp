#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  uint            seed = 10;

  hmap::Array z1 = hmap::gpu::mountain_inselberg(shape, seed);

  float       scale = 0.5f;
  hmap::Array z2 = hmap::gpu::mountain_inselberg(shape, seed, scale);

  hmap::export_banner_png("ex_mountain_inselberg.png",
                          {z1, z2},
                          hmap::Cmap::TERRAIN,
                          true);
}
