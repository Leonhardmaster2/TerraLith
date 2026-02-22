#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  uint            seed = 3;

  hmap::Array z1 = hmap::gpu::mountain_cone(shape, seed);

  hmap::export_banner_png("ex_mountain_cone.png",
                          {z1},
                          hmap::Cmap::TERRAIN,
                          true);
}
