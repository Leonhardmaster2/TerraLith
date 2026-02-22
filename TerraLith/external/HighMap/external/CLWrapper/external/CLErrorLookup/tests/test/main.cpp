/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <iostream>
#include <vector>

#include "cl_error_lookup.hpp"

int main()
{
  for (int code : {1, 0, -42, -1000})
  {
    std::cout << clerror::get_error_str(code) << "\n";
    std::cout << clerror::get_error_full(code) << "\n";
  }

  std::cout << "\n--- ERROR THROWING\n\n";
  int code = -1;
  if (code != 0) clerror::throw_opencl_error(code);

  return 0;
}
