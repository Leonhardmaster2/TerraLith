#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array source = hmap::noise_fbm(hmap::NoiseType::PERLIN,
                                       shape,
                                       res,
                                       seed++);
  hmap::Array target = hmap::noise(hmap::NoiseType::PERLIN, shape, res, seed);

  // take details of source and transfer them to target

  int         ir = 16;
  float       amplitude = 2.f;
  hmap::Array out1 = hmap::transfer(source, target, ir, amplitude);
  hmap::Array out2 = hmap::gpu::transfer(source, target, ir, amplitude);

  hmap::export_banner_png("ex_transfer.png",
                          {source, target, out1, out2},
                          hmap::Cmap::MAGMA,
                          true);
}
