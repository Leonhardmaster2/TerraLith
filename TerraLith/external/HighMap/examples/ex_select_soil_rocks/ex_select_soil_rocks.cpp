#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  uint            seed = 0;

  hmap::Array z = hmap::gpu::shattered_peak(shape, seed);
  hmap::remap(z);

  hmap::Array srocks = hmap::gpu::select_soil_rocks(z);
  hmap::remap(srocks);

  // optional -- consider increasing contrast of srocks with hmap::saturate
  if (true) hmap::saturate(srocks, 0.f, 0.3f);

  hmap::export_banner_png("ex_select_soil_rocks.png",
                          {z, srocks},
                          hmap::Cmap::JET);
}
