#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {2.f, 2.f};
  int               seed = 1;

  hmap::Array z = hmap::gpu::wavelet_noise(shape, kw, seed);

  hmap::export_banner_png("ex_wavelet_noise.png", {z}, hmap::Cmap::JET);
}
