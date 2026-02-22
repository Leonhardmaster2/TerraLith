/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <algorithm>
#include <iomanip>
#include <list>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "macrologger.h"

namespace hmap
{

std::vector<size_t> argsort(const std::vector<float> &v)
{
  // https://stackoverflow.com/questions/1577475
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::stable_sort(idx.begin(),
                   idx.end(),
                   [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });
  return idx;
}

std::string make_histogram(const std::vector<float> &values,
                           int                       bin_count,
                           int                       hist_height)
{
  std::ostringstream out;

  if (values.empty() || bin_count <= 0 || hist_height <= 0)
  {
    return "Invalid input.\n";
  }

  // Compute min and max
  auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
  float min_val = *min_it;
  float max_val = *max_it;

  // Count occurrences of exact min and max
  int count_min = std::count(values.begin(), values.end(), min_val);
  int count_max = std::count(values.begin(), values.end(), max_val);

  // Edge case: all values equal
  if (min_val == max_val)
  {
    out << "All values are equal to " << min_val << ".\n";
    out << "Count = " << values.size() << "\n\n";
    return out.str();
  }

  // Prepare bins
  std::vector<int> bins(bin_count, 0);
  float            range = max_val - min_val;
  float            inv_range = 1.0f / range;

  // Fill bins
  for (float v : values)
  {
    int idx = int((v - min_val) * inv_range * bin_count);
    if (idx == bin_count) idx = bin_count - 1; // clamp
    bins[idx]++;
  }

  // Find maximum bin height for scaling
  int   max_bin = *std::max_element(bins.begin(), bins.end());
  float scale = float(hist_height) / float(max_bin);

  // Build histogram (top to bottom)
  for (int row = hist_height; row > 0; --row)
  {
    for (int b = 0; b < bin_count; ++b)
    {
      float scaled_height = bins[b] * scale;
      out << (scaled_height >= row ? "█" : " ");
    }
    out << "\n";
  }

  // Add horizontal axis
  out << std::string(bin_count, '-') << "\n";

  // Add stats
  out << "Min value:  " << min_val << " (count = " << count_min << ")\n";
  out << "Max value:  " << max_val << " (count = " << count_max << ")\n";

  return out.str();
}

size_t upperbound_right(const std::vector<float> &v, float value)
{
  size_t idx = 0;
  for (size_t k = v.size() - 1; k > 0; k--)
    if (value > v[k])
    {
      idx = k;
      break;
    }
  return idx;
}

void reindex_vector(std::vector<int> &v, std::vector<size_t> &idx);
void reindex_vector(std::vector<float> &v, std::vector<size_t> &idx);

void vector_unique_values(std::vector<float> &v)
{
  auto last = std::unique(v.begin(), v.end());
  v.erase(last, v.end());
  std::sort(v.begin(), v.end());
  last = std::unique(v.begin(), v.end());
  v.erase(last, v.end());
}

} // namespace hmap
