#include "highmap.hpp"

int main(void)
{
  int         npts = 8;
  uint        seed = 0;
  hmap::Cloud cloud = hmap::Cloud(npts, seed);

  // create custom properties (optional)
  std::map<std::string, std::vector<float>> custom_fields;

  for (int k = 0; k < npts; ++k)
  {
    custom_fields["some_field"].push_back(float(k));
    custom_fields["custom_prop"].push_back(float(k * k));
  }

  hmap::export_points_to_ply("points.ply",
                             cloud.get_x(),
                             cloud.get_y(),
                             cloud.get_values(),
                             custom_fields);
}
