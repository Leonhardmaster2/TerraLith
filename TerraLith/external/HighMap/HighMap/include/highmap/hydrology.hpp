/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file hydrology.hpp
 * @author Otto Link (otto.link.bv@gmail.com)
 * @brief Header file for hydrological modeling functions and utilities.
 *
 * This header file declares functions and utilities for hydrological modeling,
 * including tools for computing flow directions, flow accumulation, and
 * identifying flow sinks within heightmaps. It supports multiple flow direction
 * models and the D8 model for flow direction and accumulation calculations.
 *
 * Key functionalities include:
 * - Computation of flow directions and flow accumulations using various models.
 * - Identification of flow sinks in heightmaps.
 * - Support for multiple flow direction models with customizable flow-partition
 * exponents.
 *
 * @copyright Copyright (c) 2023 Otto Link
 */
#pragma once
#include <limits>

#include "highmap/array.hpp"
#include "highmap/geometry/path.hpp"

namespace hmap
{

/**
 * @brief Computes the number of drainage paths for each cell based on the D8
 * flow direction model.
 *
 * This function calculates the number of neighboring cells that have flow
 * directions pointing to the current cell according to the D8 model. The result
 * is an array where each cell contains the count of its incoming flow
 * directions.
 *
 * @param  d8 Input array representing the flow directions according to the D8
 *            model. Each cell value indicates the direction of flow according
 *            to the D8 convention.
 * @return    Array An array where each cell contains the number of incoming
 *            flow directions pointing to it.
 */
Array d8_compute_ndip(const Array &d8);

/**
 * @brief Identifies the indices of flow sinks within the heightmap.
 *
 * This function locates the cells in the heightmap that are flow sinks (cells
 * with no outflow) and returns their indices. Flow sinks are cells that do not
 * direct flow to any other cell.
 *
 * @param z  Input array representing the heightmap values.
 * @param is Output vector containing the row indices `i` of the flow sinks.
 * @param js Output vector containing the column indices `j` of the flow sinks.
 *
 * **Example**
 * @include ex_find_flow_sinks.cpp
 */
void find_flow_sinks(const Array      &z,
                     std::vector<int> &is,
                     std::vector<int> &js);

/**
 * @brief Compute water depth for a uniform flooding level.
 *
 * Subtracts terrain elevation from a reference water level and clamps negative
 * values to zero.
 *
 * @param  z    Input elevation array.
 * @param  zref Reference water level.
 * @return      Water depth array (0 where terrain is above zref).
 *
 * **Example**
 * @include ex_flooding_from_point.cpp
 *
 * **Result**
 * @image html ex_flooding_from_point.png
 */
Array flooding_uniform_level(const Array &z, float zref);

/**
 * @brief Compute flooding starting from the lowest boundary points.
 *
 * Finds the lowest elevation on the selected boundaries and simulates flooding
 * from those points up to a reference water level.
 *
 * @param  z          Input elevation array.
 * @param  zref       Reference water level.
 * @param  from_east  Include east boundary.
 * @param  from_west  Include west boundary.
 * @param  from_north Include north boundary.
 * @param  from_south Include south boundary.
 * @return            Water depth array.
 *
 * **Example**
 * @include ex_flooding_from_point.cpp
 *
 * **Result**
 * @image html ex_flooding_from_point.png
 */
Array flooding_from_boundaries(const Array &z,
                               float        zref,
                               bool         from_east = true,
                               bool         from_west = true,
                               bool         from_north = true,
                               bool         from_south = true);

/**
 * @brief Flood terrain starting from a single seed point.
 *
 * Fills areas below a reference level by propagating from the given cell until
 * higher ground is reached.
 *
 * @param  z         Input elevation array.
 * @param  i         Seed point X index.
 * @param  j         Seed point Y index.
 * @param  depth_min Water depth at the source point (if max, uses 0).
 * @return           Water depth array.
 *
 * **Example**
 * @include ex_flooding_from_point.cpp
 *
 * **Result**
 * @image html ex_flooding_from_point.png
 */
Array flooding_from_point(const Array &z,
                          int          i,
                          int          j,
                          float depth_min = std::numeric_limits<float>::max());

/**
 * @brief Flood terrain starting from multiple seed points.
 *
 * Merges flooding results from each point below the reference level.
 *
 * @param  z         Input elevation array.
 * @param  i         List of X indices for seeds.
 * @param  j         List of Y indices for seeds.
 * @param  depth_min Water depth at the source point (if max, uses 0).
 * @return           Water depth array.
 *
 * **Example**
 * @include ex_flooding_from_point.cpp
 *
 * **Result**
 * @image html ex_flooding_from_point.png
 */
Array flooding_from_point(const Array            &z,
                          const std::vector<int> &i,
                          const std::vector<int> &j,
                          float depth_min = std::numeric_limits<float>::max());

/**
 * @brief Estimate lake water depths on a terrain by filling depressions.
 *
 * This function identifies depressions in a terrain elevation model and
 * simulates the flooding of these areas to produce a lake system. It uses a
 * rough depression-filling algorithm to compute the water surface, then
 * subtracts the original elevations to obtain the water depth at each cell.
 *
 * @param  z                 Input 2D array representing terrain elevations
 *                           (height field).
 * @param  iterations        Maximum number of iterations allowed for the
 *                           depression-filling algorithm.
 * @param  epsilon           Convergence tolerance for the depression-filling
 *                           step.
 * @param  surface_threshold The minimum number of pixels a component must have
 *                           to be retained. Components smaller than this
 *                           threshold will be removed. The default value is 0
 *                           (no filtering).
 *
 * @return                   A 2D array representing the water depth for each
 *                           cell. Values are zero where no lake is present and
 *                           positive where water accumulates in depressions.
 *
 * @see                      depression_filling
 *
 * **Example**
 * @include ex_flooding_lake_system.cpp
 *
 * **Result**
 * @image html ex_flooding_lake_system.png
 */
Array flooding_lake_system(const Array &z,
                           int          iterations = 500,
                           float        epsilon = 1e-3f,
                           float        surface_threshold = 0);

/**
 * @brief Computes the flow accumulation for each cell using the D8 flow
 * direction model.
 *
 * This function calculates the flow accumulation for each cell in the heightmap
 * based on the D8 flow direction model \cite Zhou2019. Flow accumulation
 * represents the total amount of flow that accumulates at each cell from
 * upstream cells.
 *
 * @param  z Input array representing the heightmap values.
 * @return   Array An array where each cell contains the computed flow
 *           accumulation.
 *
 * **Example**
 * @include ex_flow_accumulation_d8.cpp
 *
 * **Result**
 * @image html ex_flow_accumulation_d80.png
 * @image html ex_flow_accumulation_d81.png
 *
 * @see      flow_direction_d8
 */
Array flow_accumulation_d8(const Array &z);

/**
 * @brief Computes the flow accumulation for each cell using the Multiple Flow
 * Direction (MFD) model.
 *
 * This function calculates the flow accumulation for each cell based on the MFD
 * model \cite Qin2007. Flow accumulation represents the total amount of flow
 * that accumulates at each cell from upstream cells. The flow-partition
 * exponent is locally defined using a reference talus value, where smaller
 * values of `talus_ref` will lead to narrower flow streams. The maximum talus
 * value of the heightmap can be used as a reference.
 *
 * @param  z         Input array representing the heightmap values.
 * @param  talus_ref Reference talus used to locally define the flow-partition
 *                   exponent. Small values will result in thinner flow streams.
 * @return           Array An array where each cell contains the computed flow
 *                   accumulation.
 *
 * **Example**
 * @include ex_flow_accumulation_dinf.cpp
 *
 * **Result**
 * @image html ex_flow_accumulation_dinf0.png
 * @image html ex_flow_accumulation_dinf1.png
 *
 * @see              flow_direction_dinf, flow_accumulation_d8
 */
Array flow_accumulation_dinf(const Array &z, float talus_ref);

/**
 * @brief Computes the flow direction from each cell to its downslope neighbor
 * using the D8 model.
 *
 * This function calculates the direction of flow for each cell in the heightmap
 * using the D8 flow direction model @cite Greenlee1987. The D8 model defines
 * eight possible flow directions as follows:
 * @verbatim 5 6 7 4 . 0 3 2 1
 * @endverbatim
 *
 * @param  z Input array representing the heightmap values.
 * @return   Array An array where each cell contains the flow direction
 *           according to the D8 nomenclature.
 *
 * @see      flow_accumulation_d8
 */
Array flow_direction_d8(const Array &z);

/**
 * @brief Computes the flow direction and weights for each direction using the
 * Multiple Flow Direction (MFD) model.
 *
 * This function calculates the flow direction for each cell and provides the
 * weight for each possible flow direction using the MFD model \cite Qin2007.
 * The flow-partition exponent is determined using a reference talus value.
 * Smaller values of `talus_ref` will lead to thinner flow streams. The maximum
 * talus value of the heightmap can be used as a reference.
 *
 * @param  z         Input array representing the heightmap values.
 * @param  talus_ref Reference talus used to locally define the flow-partition
 *                   exponent. Smaller values will result in thinner flow
 *                   streams.
 * @return           std::vector<Array> A vector of arrays, each containing the
 *                   weights for flow directions at every cell.
 */
std::vector<Array> flow_direction_dinf(const Array &z, float talus_ref);

/**
 * @brief Computes the flow direction using the Multiple Flow Direction (MFD)
 * model.
 */
Array flow_direction_dinf_angle(const Array &z, float talus_ref);

/**
 * @brief Computes the optimal flow path from a starting point to the boundary
 * of a given elevation array.
 *
 * This function finds the flow path on a grid represented by the input array
 * `z`, starting from the given point `ij_start`. It identifies the best path to
 * the boundary, minimizing upward elevation penalties while accounting for
 * distance and elevation factors.
 *
 * @param  z                   The input 2D array representing elevation values.
 * @param  ij_start            The starting point as a 2D vector of indices (i,
 *                             j) within the array.
 * @param  elevation_ratio     Weight for elevation difference in the cost
 *                             function (default: 0.5).
 * @param  distance_exponent   Exponent for the distance term in the cost
 *                             function (default: 2.0).
 * @param  upward_penalization Penalty factor for upward elevation changes
 *                             (default: 100.0).
 * @return                     A Path object representing the optimal flow path
 *                             with normalized x and y coordinates and
 *                             corresponding elevations.
 *
 * The output path consists of:
 * - Normalized x-coordinates along the path.
 * - Normalized y-coordinates along the path.
 * - Elevation values corresponding to each point on the path.
 *
 * **Example**
 * @include ex_flow_stream.cpp
 *
 * **Result**
 * @image html ex_flow_stream.png
 */
Path flow_stream(const Array    &z,
                 const Vec2<int> ij_start,
                 const float     elevation_ratio = 0.5f,
                 const float     distance_exponent = 2.f,
                 const float     upward_penalization = 100.f);

/**
 * @brief Generates a 2D array representing a riverbed based on a specified
 * path.
 *
 * This function calculates a scalar depth field (`dz`) for a riverbed shape
 * using a path, which can optionally be smoothed with Bezier curves. It
 * supports noise perturbation and post-filtering to adjust the riverbed's
 * features.
 *
 * @param  path                 The input path defining the riverbed's
 *                              trajectory.
 * @param  shape                The dimensions of the output array (width,
 *                              height).
 * @param  bbox                 The bounding box for the output grid in world
 *                              coordinates.
 * @param  bezier_smoothing     Flag to enable or disable Bezier smoothing of
 *                              the path.
 * @param  depth_start          The depth at the start of the riverbed.
 * @param  depth_end            The depth at the end of the riverbed.
 * @param  slope_start          The slope multiplier at the start of the
 *                              riverbed.
 * @param  slope_end            The slope multiplier at the end of the riverbed.
 * @param  shape_exponent_start The shape exponent at the start of the riverbed.
 * @param  shape_exponent_end   The shape exponent at the end of the riverbed.
 * @param  k_smoothing          The smoothing factor for the riverbed shape
 *                              adjustments.
 * @param  post_filter_ir       The radius of the post-filtering operation for
 *                              smoothing the output.
 * @param  p_noise_x            Optional pointer to a noise array for perturbing
 *                              the x-coordinates.
 * @param  p_noise_y            Optional pointer to a noise array for perturbing
 *                              the y-coordinates.
 * @param  p_noise_r            Optional pointer to a noise array for perturbing
 *                              the radial function.
 * @return                      A 2D array representing the calculated riverbed
 *                              depth field.
 *
 * @note The function requires the path to have at least two points. If the path
 * has fewer points, an empty array is returned with the given shape.
 *
 * **Example**
 * @include ex_generate_riverbed.cpp
 *
 * **Result**
 * @image html ex_generate_riverbed.png
 */
Array generate_riverbed(const Path &path,
                        Vec2<int>   shape,
                        Vec4<float> bbox = {0.f, 1.f, 0.f, 1.f},
                        bool        bezier_smoothing = false,
                        float       depth_start = 0.01f,
                        float       depth_end = 1.f,
                        float       slope_start = 64.f,
                        float       slope_end = 32.f,
                        float       shape_exponent_start = 1.f,
                        float       shape_exponent_end = 10.f,
                        float       k_smoothing = 0.5f,
                        int         post_filter_ir = 0,
                        Array      *p_noise_x = nullptr,
                        Array      *p_noise_y = nullptr,
                        Array      *p_noise_r = nullptr);

/**
 * @brief Merge two water depth fields.
 *
 * Computes the maximum (or smoothed maximum) of two depth arrays.
 *
 * @param  depth1   First water depth array.
 * @param  depth2   Second water depth array.
 * @param  k_smooth Smoothing parameter (0 = sharp max).
 * @return          Combined water depth array.
 */
Array merge_water_depths(const Array &depth1,
                         const Array &depth2,
                         float        k_smooth = 0.f);

/**
 * @brief Compute water depth over a masked terrain using harmonic
 * interpolation.
 *
 * This function estimates the water depth above a terrain surface by solving a
 * Laplace equation on the domain defined by @p mask. The solution is obtained
 * using the harmonic interpolation method with Successive Over-Relaxation
 * (SOR). The resulting water depth is given by the difference between the
 * interpolated surface and the original terrain elevation.
 *
 * @param  z              Input 2D array representing the terrain elevations
 *                        (height field).
 * @param  mask           Mask array of the same shape as @p z. Values greater
 *                        than @p mask_threshold define regions where water can
 *                        accumulate, while lower values represent boundaries or
 *                        fixed terrain.
 * @param  mask_threshold Threshold used to convert @p mask into a binary field
 *                        (0 or 1) for identifying water/terrain boundaries.
 * @param  iterations_max Maximum number of SOR iterations used in the harmonic
 *                        interpolation.
 * @param  tolerance      Convergence criterion: the algorithm stops if the
 *                        maximum absolute update between iterations is less
 *                        than this value.
 * @param  omega          Relaxation factor for the SOR solver (1 < omega < 2
 *                        recommended).
 *
 * @return                A 2D array containing the computed water depth at each
 *                        grid cell. Depth values are non-negative where water
 *                        is present and zero where the mask indicates no water.
 *
 * @see                   harmonic_interpolation
 *
 * **Example**
 * @include ex_water_depth_from_mask.cpp
 *
 * **Result**
 * @image html ex_water_depth_from_mask.png
 */
Array water_depth_from_mask(const Array &z,
                            const Array &mask,
                            float        mask_threshold = 0.f,
                            int          iterations_max = 10000,
                            float        tolerance = 1e-2f,
                            float        omega = 1.8f);

/**
 * @brief Apply a drying factor to a water depth field.
 *
 * This function reduces the water depth values in-place by multiplying them by
 * a given @p dry_out_ratio. If a mask is provided, the drying is applied only
 * where the mask is non-zero, allowing selective drying of specific regions.
 *
 * @param water_depth   Reference to the 2D array containing water depth values.
 *                      The array is modified in-place.
 * @param dry_out_ratio Multiplicative factor (typically between 0 and 1) used
 *                      to scale down the water depth. A value of 1 leaves the
 *                      depth unchanged, while 0 removes all water.
 * @param p_mask        Optional pointer to a mask array of the same shape as @p
 *                      water_depth. Drying is applied only where the mask has
 *                      non-zero values. If `nullptr`, the factor is applied
 *                      uniformly to all cells.
 * @param depth_max     Maximum water depth, computed automatically by default.
 *
 * **Example**
 * @include ex_water_depth_dry_out.cpp
 *
 * **Result**
 * @image html ex_water_depth_dry_out.png
 */
void water_depth_dry_out(Array       &water_depth,
                         float        dry_out_ratio = 0.5f,
                         const Array *p_mask = nullptr,
                         float depth_max = std::numeric_limits<float>::max());

/**
 * @brief Simulates the increase in water depth over a terrain.
 *
 * This function propagates additional water depth over a terrain elevation map
 * (`z`), starting from cells that already contain water. The propagation occurs
 * only in the upward direction (i.e., to higher elevation cells) and considers
 * 8-neighbor connectivity. It effectively models how water would expand when
 * its level rises by `additional_depth`.
 *
 * @param  water_depth      Input array representing the base water depth.
 * @param  z                Elevation array corresponding to the same grid as
 *                          `water_depth`.
 * @param  additional_depth The additional water depth to simulate (e.g., a
 *                          flooding increment).
 * @return                  An Array representing the updated water depth
 *                          distribution after propagation.
 *
 * @note The algorithm uses a simple flood-fill–like approach with an upward
 * constraint, ensuring that water spreads only to neighboring cells at higher
 * elevation.
 */
Array water_depth_increase(const Array &water_depth,
                           const Array &z,
                           float        additional_depth);

/**
 * @brief Generates a binary mask representing water presence.
 *
 * This version of the function converts the given water depth array into a
 * binary mask where non-zero values indicate the presence of water.
 *
 * @param  water_depth Input array representing water depth values.
 * @return             A binary Array where each cell is 1 if water is present,
 *                     0 otherwise.
 *
 * **Example**
 * @include ex_water_mask.cpp
 *
 * **Result**
 * @image html ex_water_mask.png
 */
Array water_mask(const Array &water_depth);

/**
 * @brief Computes a gradient-based water mask using an extended water depth
 * model.
 *
 * This function computes a smooth mask indicating regions that would be newly
 * flooded when the water depth is artificially increased by a specified
 * additional amount. It uses `water_depth_increase()` to simulate the spread of
 * water over the terrain.
 *
 * @param  water_depth      Input array representing the current water depth.
 * @param  z                Elevation array corresponding to the same grid as
 *                          `water_depth`.
 * @param  additional_depth The amount of additional water depth to simulate.
 * @return                  An Array representing the normalized water extension
 *                          mask, where values range from 0 to 1.
 *
 * **Example**
 * @include ex_water_mask.cpp
 *
 * **Result**
 * @image html ex_water_mask.png
 */
Array water_mask(const Array &water_depth,
                 const Array &z,
                 float        additional_depth);

} // namespace hmap

namespace hmap::gpu
{

/*! @brief See hmap::flow_direction_d8 */
Array flow_direction_d8(const Array &z);

/*! @brief See hmap::generate_riverbed */
Array generate_riverbed(const Path &path,
                        Vec2<int>   shape,
                        Vec4<float> bbox = {0.f, 1.f, 0.f, 1.f},
                        bool        bezier_smoothing = false,
                        float       depth_start = 0.01f,
                        float       depth_end = 1.f,
                        float       slope_start = 64.f,
                        float       slope_end = 32.f,
                        float       shape_exponent_start = 1.f,
                        float       shape_exponent_end = 10.f,
                        float       k_smoothing = 0.5f,
                        int         post_filter_ir = 0,
                        Array      *p_noise_x = nullptr,
                        Array      *p_noise_y = nullptr,
                        Array      *p_noise_r = nullptr);

} // namespace hmap::gpu
