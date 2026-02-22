#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int> shape = {256, 256};
  uint            seed = 0;

  hmap::Array z = hmap::gpu::shattered_peak(shape, seed);
  hmap::remap(z);

  // weathered
  int         ir_curvature = 0;
  int         ir_gradient = 4;
  hmap::Array sw = hmap::gpu::select_soil_weathered(
      z,
      ir_curvature,
      ir_gradient,
      hmap::ClampMode::POSITIVE_ONLY,
      1.f,
      1.f,
      0.2f);

  // flow
  hmap::Array sflow = hmap::gpu::select_soil_flow(z);

  hmap::remap(z);
  hmap::remap(sw);
  hmap::remap(sflow);

  z.dump("out0.png");
  sflow.dump("out1.png");

  hmap::export_banner_png("ex_select_soil_weathered.png",
                          {z, sw, sflow},
                          hmap::Cmap::JET);
}
