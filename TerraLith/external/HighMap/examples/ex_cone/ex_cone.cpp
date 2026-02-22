#include "highmap.hpp"

int main(void)
{
  hmap::Vec2<int> shape = {256, 256};

  hmap::Array z1 = hmap::cone(shape, 4.f);
  hmap::Array z2 = hmap::cone_sigmoid(shape, 0.8f);
  hmap::Array z3 = hmap::cone_complex(shape, 1.2f);

  hmap::export_banner_png("ex_cone.png", {z1, z2, z3}, hmap::Cmap::MAGMA);
}
