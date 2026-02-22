#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  uint            seed = 3;

  hmap::Array z1 = hmap::gpu::shattered_peak(shape, seed);

  hmap::export_banner_png("ex_shattered_peak.png",
                          {z1},
                          hmap::Cmap::TERRAIN,
                          true);
}
