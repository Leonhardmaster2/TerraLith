#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {16.f, 16.f};
  int               seed = 0;

  hmap::Array z1 = hmap::gpu::hemisphere_field(shape, kw, seed, 0.01f, 1.f);
  hmap::remap(z1);

  hmap::Array z2 = hmap::gpu::hemisphere_field_fbm(shape, kw, seed);
  hmap::remap(z2);

  hmap::export_banner_png("ex_hemisphere_field.png",
                          {z1, z2},
                          hmap::Cmap::INFERNO);
}
