#include "highmap.hpp"

int main(void)
{
  hmap::gpu::init_opencl();

  hmap::Vec2<int>   shape = {256, 256};
  hmap::Vec2<float> kw = {4.f, 4.f};
  uint              seed = 0;

  hmap::Array noise = hmap::noise_fbm(hmap::NoiseType::SIMPLEX2,
                                      shape,
                                      kw,
                                      seed++,
                                      8,
                                      0.f);

  hmap::remap(noise);

  // use dedicated mask generator
  float           radius = 0.3f;
  float           displacement = 0.2f;
  hmap::NoiseType noise_type = hmap::NoiseType::SIMPLEX2S;
  hmap::Array     land_mask = hmap::island_land_mask(shape,
                                                 radius,
                                                 seed,
                                                 displacement,
                                                 noise_type);

  // default noise
  hmap::Array za = hmap::gpu::island(land_mask, seed);

  // custom noise (expected in [0, 1])
  hmap::Array zb = hmap::gpu::island(land_mask, &noise);

  // use "any" noise as land mask
  auto land_mask2 = noise;
  hmap::zeroed_edges(land_mask2);
  hmap::make_binary(land_mask2, 0.15f);
  hmap::Array zc = hmap::gpu::island(land_mask2, seed);

  hmap::export_banner_png("ex_island.png",
                          {land_mask, za, zb, zc},
                          hmap::Cmap::TERRAIN,
                          true);
}
