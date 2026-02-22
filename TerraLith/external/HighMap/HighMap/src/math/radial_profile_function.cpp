/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <stdexcept>

#include "macrologger.h"

#include "highmap/math.hpp"
#include "highmap/operator.hpp"

namespace hmap
{

std::function<float(float)> get_radial_profile_function(
    RadialProfile radial_profile,
    float         delta)
{

  std::function<float(float)> fct;

  // expects x in [0..1] => output in [0..1] with 0 at x = 0 and 1 at
  // x = 1
  switch (radial_profile)
  {

  case RadialProfile::RP_GAIN:
    fct = [delta](float x) { return gain(x, delta); };
    break;

  case RadialProfile::RP_LINEAR: fct = [](float x) { return x; }; break;

  case RadialProfile::RP_POW:
    fct = [delta](float x) { return std::pow(x, delta); };
    break;

  case RadialProfile::RP_SMOOTHSTEP:
    fct = [](float x) { return smoothstep3(x); };
    break;

  case RadialProfile::RP_SMOOTHSTEP_UPPER:
    fct = [](float x) { return smoothstep3_upper(x); };
    break;

  default:
    throw std::invalid_argument(
        "Invalid radial profile request in hmap::get_radial_profile_function");
  }

  return fct;
}

} // namespace hmap
