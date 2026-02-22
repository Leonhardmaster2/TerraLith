#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int> shape = {256, 256};

  std::vector<hmap::PrimitiveType> types = {
      hmap::PrimitiveType::PRIM_BIQUAD_PULSE,
      hmap::PrimitiveType::PRIM_BUMP,
      hmap::PrimitiveType::PRIM_CONE,
      hmap::PrimitiveType::PRIM_CONE_SMOOTH,
      hmap::PrimitiveType::PRIM_CUBIC_PULSE,
      hmap::PrimitiveType::PRIM_SMOOTH_COSINE};

  std::vector<hmap::Array> zs = {};

  for (auto type : types)
  {
    hmap::Array z = hmap::get_primitive_base(type, shape);
    zs.push_back(z);
  }

  hmap::export_banner_png("ex_get_primitive_base.png", zs, hmap::Cmap::INFERNO);
}
