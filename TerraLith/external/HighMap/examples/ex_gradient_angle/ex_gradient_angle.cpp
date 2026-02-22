#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);

  auto a0 = hmap::gradient_angle(z);

  int  ir = 16;
  auto a1 = hmap::gradient_angle_circular_smoothing(z, ir);

  auto  a2 = a0;
  float talus = M_PI;
  float talus_width = 0.01f * talus;
  ir = 16;

  hmap::smooth_cpulse_edge_removing(a2, talus, talus_width, ir);

  hmap::export_banner_png("ex_gradient_angle.png",
                          {z, a0, a1, a2},
                          hmap::Cmap::INFERNO,
                          false,
                          true);
}
