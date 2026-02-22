#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> res = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, res, seed);
  int         nbuffer = 64;

  auto zp = z;
  hmap::make_periodic(zp, nbuffer);

  auto zp_x = z;
  auto zp_y = z;

  hmap::make_periodic(zp_x, nbuffer, hmap::PeriodicityType::PERIODICITY_X);
  hmap::make_periodic(zp_y, nbuffer, hmap::PeriodicityType::PERIODICITY_Y);

  // do some tiling to check periodicity
  auto zt = zp;
  zt = hstack(zt, zt);
  zt = vstack(zt, zt);

  hmap::export_banner_png("ex_make_periodic0.png",
                          {z, zp, zp_x, zp_y},
                          hmap::Cmap::VIRIDIS);

  zt.to_png("ex_make_periodic1.png", hmap::Cmap::VIRIDIS);
}
