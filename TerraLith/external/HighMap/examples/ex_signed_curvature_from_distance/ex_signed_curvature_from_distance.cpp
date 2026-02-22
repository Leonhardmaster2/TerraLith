#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  hmap::clamp_min(z, 0.f);

  int ir = 32; // can be 0

  auto d = hmap::distance_transform(z);
  auto sc = hmap::signed_curvature_from_distance(z, ir);
  auto sd = hmap::signed_distance_transform(z, ir);

  z.to_png("ex_signed_curvature_from_distance0.png", hmap::Cmap::JET);
  d.to_png("ex_signed_curvature_from_distance1.png", hmap::Cmap::JET);
  sc.to_png("ex_signed_curvature_from_distance2.png", hmap::Cmap::JET);
  sd.to_png("ex_signed_curvature_from_distance3.png", hmap::Cmap::JET);
}
