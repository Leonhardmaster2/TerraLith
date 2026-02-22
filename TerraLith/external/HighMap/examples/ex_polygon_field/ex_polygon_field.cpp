#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {4.f, 4.f};
  int               seed = 1;

  hmap::Array z1 = hmap::gpu::polygon_field(shape, kw, seed);
  hmap::remap(z1);

  hmap::Array z2 = hmap::gpu::polygon_field_fbm(shape, kw, seed);
  hmap::remap(z2);

  hmap::export_banner_png("ex_polygon_field.png",
                          {z1, z2},
                          hmap::Cmap::INFERNO);
}
