#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {2.f, 2.f};
  int               seed = 0;

  hmap::Array z = hmap::noise_fbm(hmap::NoiseType::PERLIN, shape, kw, seed);
  hmap::remap(z);

  hmap::Array dr = hmap::noise_fbm(hmap::NoiseType::PERLIN,
                                   shape,
                                   {16.f, 16.f},
                                   ++seed,
                                   8,
                                   0.1f);
  hmap::remap(dr, -0.2f, 0.9f);

  // --- generate path

  hmap::Path path = hmap::find_cut_path_dijkstra(
      z,
      hmap::DomainBoundary::BOUNDARY_LEFT,
      hmap::DomainBoundary::BOUNDARY_RIGHT);

  path.decimate_vw(20);
  path.bspline();

  hmap::Array zp = hmap::Array(shape);
  path.to_array(zp);

  // --- carve

  float bottom_extent = int(shape.x / 32);
  float vmin = 0.1f;
  float depth = 0.05f;
  float falloff_distance = 4 * bottom_extent;
  float outer_slope = 0.1f;
  bool  preserve_bedshape = true;
  auto  radial_profile = hmap::RadialProfile::RP_GAIN;
  float radial_profile_parameter = 2.f;

  hmap::Array zf = z;
  hmap::Array mask;

  hmap::flatbed_carve(zf,
                      path,
                      bottom_extent,
                      vmin,
                      depth,
                      falloff_distance,
                      outer_slope,
                      preserve_bedshape,
                      radial_profile,
                      radial_profile_parameter,
                      &mask,
                      &dr);

  hmap::export_banner_png("ex_flatbed_carve.png",
                          {z, zp, mask, zf},
                          hmap::Cmap::INFERNO);
}
