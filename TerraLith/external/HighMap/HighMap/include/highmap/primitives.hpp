/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file primitives.hpp
 * @author Otto Link (otto.link.bv@gmail.com)
 * @brief Core procedural primitives for heightmap generation, including noise
 * functions (Perlin, Gabor, Voronoi, Phasor), terrain features (hills, craters,
 * calderas, dunes), geometric shapes (disk, rectangle), and advanced patterns
 * (DLA, Dendry). Supports both CPU and GPU-accelerated generation for complex
 * terrain synthesis.
 *
 * @copyright Copyright (c) 2023
 */
#pragma once

#include "FastNoiseLite.h"

#include "highmap/array.hpp"
#include "highmap/erosion.hpp"
#include "highmap/functions.hpp"

#define HMAP_GRADIENT_OFFSET 0.001f

namespace hmap
{

/**
 * @enum VoronoiReturnType
 * @brief Selects the value returned by the Voronoi evaluation.
 *
 * F1 is the distance to the nearest site, F2 to the second nearest.
 */
enum VoronoiReturnType : int
{
  F1_SQUARED,            ///< Returns F1^2
  F2_SQUARED,            ///< Returns F2^2
  F1TF2_SQUARED,         ///< Returns (F1 * F2)^2
  F1DF2_SQUARED,         ///< Returns (F1 / F2)^2
  F2MF1_SQUARED,         ///< Returns (F2 - F1)^2
  EDGE_DISTANCE_EXP,     ///< Exponential edge distance
  EDGE_DISTANCE_SQUARED, ///< Squared edge distance
  CONSTANT,              ///< Constant value
  CONSTANT_F2MF1_SQUARED ///< Constant × (F2 - F1)^2
};

/**
 * @enum PrimitiveType
 * @brief Defines the primitive shape used for synthesis.
 */
enum PrimitiveType : int
{
  PRIM_BIQUAD_PULSE,
  PRIM_BUMP,
  PRIM_CONE,
  PRIM_CONE_SMOOTH,
  PRIM_CUBIC_PULSE,
  PRIM_SMOOTH_COSINE,
};

/**
 * @brief Generates a primitive shape as a 2D array.
 *
 * @param  primitive_type Type of primitive to generate.
 * @param  shape          Output array resolution.
 * @param  p_noise_x      Optional X-direction noise modulation.
 * @param  p_noise_y      Optional Y-direction noise modulation.
 * @param  center         Center position of the primitive (normalized).
 * @param  bbox           Bounding box of the primitive (normalized).
 * @return                The generated primitive as an Array.
 */
Array get_primitive_base(const PrimitiveType &primitive_type,
                         const Vec2<int>     &shape,
                         const Array         *p_noise_x = nullptr,
                         const Array         *p_noise_y = nullptr,
                         Vec2<float>          center = {0.5f, 0.5f},
                         Vec4<float>          bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a 'biquadratic pulse'.
 *
 * @param  shape                Array shape.
 * @param  gain                 Gain (the higher, the steeper).
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the gain parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Perlin billow noise.
 *
 * **Example**
 * @include ex_biquad_pulse.cpp
 *
 * **Result**
 * @image html ex_biquad_pulse.png
 */
Array biquad_pulse(Vec2<int>    shape,
                   float        gain = 1.f,
                   const Array *p_ctrl_param = nullptr,
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   const Array *p_stretching = nullptr,
                   Vec2<float>  center = {0.5f, 0.5f},
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a bump.
 *
 * @param  shape                Array shape.
 * @param  gain                 Gain (the higher, the steeper the bump).
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the gain parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Perlin billow noise.
 *
 * **Example**
 * @include ex_bump.cpp
 *
 * **Result**
 * @image html ex_bump.png
 */
Array bump(Vec2<int>    shape,
           float        gain = 1.f,
           const Array *p_ctrl_param = nullptr, // gain multiplier
           const Array *p_noise_x = nullptr,
           const Array *p_noise_y = nullptr,
           const Array *p_stretching = nullptr,
           Vec2<float>  center = {0.5f, 0.5f},
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a 2D Lorentzian bump pattern.
 *
 * This function fills an array with values computed from a normalized
 * Lorentzian-shaped bump function. The bump is centered at a given position,
 * has a specified radius, and can vary in width depending on a control
 * parameter. Optional noise arrays can be applied to perturb the x and y
 * coordinates before evaluation.
 *
 * The Lorentzian function is normalized so that it returns 1.0 at the center
 * and smoothly decays to 0.0 at the radius boundary. Outside the radius, the
 * value is exactly 0.
 *
 * @param  shape        Dimensions of the output array (width, height).
 * @param  width_factor Scaling factor controlling the bump width relative to
 *                      the control parameter.
 * @param  radius       Radius of the bump in the coordinate space of @p bbox.
 * @param  p_ctrl_param Optional pointer to an array of per-pixel control
 *                      parameters. If provided, it modulates the width of the
 *                      Lorentzian bump.
 * @param  p_noise_x    Optional pointer to an array of x-offset noise values.
 *                      Values are added to the x coordinate before evaluation.
 * @param  p_noise_y    Optional pointer to an array of y-offset noise values.
 *                      Values are added to the y coordinate before evaluation.
 * @param  center       Center of the bump in the coordinate space of @p bbox.
 * @param  bbox         Bounding box (xmin, ymin, xmax, ymax) defining the
 *                      coordinate space for the array.
 *
 * @return              A new Array object of size @p shape containing the bump
 *                      pattern.
 *
 * @note The function normalizes the Lorentzian curve so that the maximum value
 * is 1 at the center and smoothly decays to 0 at the boundary defined by
 *       @p radius.
 *
 * @see                 fill_array_using_xy_function
 *
 * **Example**
 * @include ex_bump.cpp
 *
 * **Result**
 * @image html ex_bump.png
 */
Array bump_lorentzian(
    Vec2<int>    shape,
    float        shape_factor = 0.5f,
    float        radius = 0.5f,
    const Array *p_ctrl_param = nullptr, // shape_factor multiplier
    const Array *p_noise_x = nullptr,
    const Array *p_noise_y = nullptr,
    Vec2<float>  center = {0.5f, 0.5f},
    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a caldera-shaped heightmap.
 *
 * @param  shape         Array shape.
 * @param  radius        Crater radius at the ridge.
 * @param  sigma_inner   Inner half-width.
 * @param  sigma_outer   Outer half-width.
 * @param  z_bottom      Bottom elevation (ridge is at elevation `1`).
 * @param  p_noise       Displacement noise.
 * @param  noise_amp_r   Radial noise absolute scale (in pixels).
 * @param  noise_ratio_z Vertical noise relative scale (in [0, 1]).
 * @param  center        Primitive reference center.
 * @param  bbox          Domain bounding box.
 * @return               Array Resulting array.
 *
 * **Example**
 * @include ex_caldera.cpp
 *
 * **Result**
 * @image html ex_caldera.png
 */
Array caldera(Vec2<int>    shape,
              float        radius,
              float        sigma_inner,
              float        sigma_outer,
              float        z_bottom,
              const Array *p_noise,
              float        noise_amp_r,
              float        noise_ratio_z,
              Vec2<float>  center = {0.5f, 0.5f},
              Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

Array caldera(Vec2<int>   shape,
              float       radius,
              float       sigma_inner,
              float       sigma_outer,
              float       z_bottom,
              Vec2<float> center = {0.5f, 0.5f},
              Vec4<float> bbox = {0.f, 1.f, 0.f, 1.f}); ///< @overload

/**
 * @brief Return a checkerboard heightmap.
 *
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumber with respect to a unit domain.
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_checkerboard.cpp
 *
 * **Result**
 * @image html ex_checkerboard.png
 */
Array checkerboard(Vec2<int>    shape,
                   Vec2<float>  kw,
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   const Array *p_stretching = nullptr,
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a synthetic conical mountain heightmap.
 *
 * This function creates a simple cone-shaped elevation field centered at a
 * given position. The cone profile follows a linear slope until it reaches the
 * talus angle, resembling volcanic cones or alluvial fans. Optional
 * displacement noise fields can be applied to perturb the cone shape.
 *
 * @param  shape     Output array shape (resolution in x and y).
 * @param  slope     Slope angle of the cone (controls steepness of the sides).
 * @param  center    Center of the cone in normalized coordinates (default =
 *                   {0.5f, 0.5f}).
 * @param  p_noise_x Optional pointer to external displacement noise field
 *                   (X-axis).
 * @param  p_noise_y Optional pointer to external displacement noise field
 *                   (Y-axis).
 * @param  bbox      Bounding box of the generation domain in normalized
 *                   coordinates (default = {0.f, 1.f, 0.f, 1.f}).
 *
 * @return           Array containing the generated conical heightmap.
 *
 * **Example**
 * @include ex_cone.cpp
 *
 * **Result**
 * @image html ex_cone.png
 */
Array cone(Vec2<int>    shape,
           float        slope,
           float        apex_elevation = 1.f,
           bool         smooth_profile = false,
           Vec2<float>  center = {0.5f, 0.5f},
           const Array *p_noise_x = nullptr,
           const Array *p_noise_y = nullptr,
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a complex conical heightfield with valleys, directional
 * bias, and radial waviness.
 *
 * This function procedurally constructs an island-like cone shape inside a 2D
 * array. The height at each position is determined from the normalized radial
 * distance to the center, with optional modulation by valleys, directional
 * bias, and radial waviness. An erosion profile is applied to shape the
 * valleys, and an optional control map can modulate the valley amplitude
 * locally. The resulting height values are clamped to [0, 1].
 *
 * @param  shape               Output array dimensions (width, height).
 * @param  alpha               Exponent controlling the steepness of the cone
 *                             slope.
 * @param  radius              Effective radius of the island (in coordinate
 *                             space units).
 * @param  valley_amp          Global amplitude of valley depressions.
 * @param  valley_nb           Number of valleys around the island.
 * @param  valley_decay_ratio  Controls how valley amplitude decays toward the
 *                             center.
 * @param  valley_angle0       Angular offset of the first valley (in degrees).
 * @param  erosion_profile     Erosion profile type used for shaping valley
 *                             cross-sections.
 * @param  erosion_delta       Smoothing parameter for the erosion profile
 *                             function.
 * @param  radial_waviness_amp Amplitude of radial sinusoidal perturbations
 *                             (coastal waviness).
 * @param  radial_waviness_kw  Frequency multiplier for radial waviness.
 * @param  bias_angle          Direction (in degrees) of the slope bias (e.g.
 *                             wind or sun exposure).
 * @param  bias_amp            Amplitude of the directional bias effect.
 * @param  bias_exponent       Controls how bias strength varies with radius.
 * @param  center              Center position of the cone in coordinate space.
 * @param  p_ctrl_param        Optional control array; its values modulate the
 *                             local valley amplitude. If nullptr, the valley
 *                             amplitude is uniform.
 * @param  p_noise_x           Optional X-displacement noise field for
 *                             coordinate perturbation.
 * @param  p_noise_y           Optional Y-displacement noise field for
 *                             coordinate perturbation.
 * @param  bbox                Bounding box defining the world coordinates
 *                             covered by the array.
 *
 * @return                     A 2D Array containing the generated heightfield
 *                             values in the range [0, 1].
 *
 * **Example**
 * @include ex_cone.cpp
 *
 * **Result**
 * @image html ex_cone.png
 */

Array cone_complex(
    Vec2<int>             shape,
    float                 alpha,
    float                 radius = 0.5f,
    bool                  smooth_profile = true,
    float                 valley_amp = 0.2f,
    int                   valley_nb = 5,
    float                 valley_decay_ratio = 0.5f,
    float                 valley_angle0 = 15.f,
    const ErosionProfile &erosion_profile = ErosionProfile::TRIANGLE_GRENIER,
    float                 erosion_delta = 0.01f,
    float                 radial_waviness_amp = 0.05f,
    float                 radial_waviness_kw = 2.f,
    float                 bias_angle = 30.f,
    float                 bias_amp = 0.75f,
    float                 bias_exponent = 1.f,
    Vec2<float>           center = {0.5f, 0.5f},
    const Array          *p_ctrl_param = nullptr,
    const Array          *p_noise_x = nullptr,
    const Array          *p_noise_y = nullptr,
    Vec4<float>           bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a smooth conical heightmap using a sigmoid-based profile.
 *
 * This function creates a 2D array representing a cone shape centered at a
 * given position. The height decreases radially from the apex according to a
 * sigmoid-shaped falloff, producing a smooth, rounded cone rather than a sharp
 * linear one. Optionally, displacement noise can be applied along the X and Y
 * axes to perturb the cone surface.
 *
 * @param  shape     Dimensions of the output array (width, height).
 * @param  alpha     Peak elevation value of the cone (controls maximum height).
 * @param  radius    Radius of the cone base, expressed in normalized [0, 1]
 *                   units relative to the bounding box.
 * @param  center    Normalized coordinates of the cone’s center (default =
 *                   {0.5, 0.5}).
 * @param  p_noise_x Optional pointer to an array representing horizontal
 *                   displacement noise (nullptr for none).
 * @param  p_noise_y Optional pointer to an array representing vertical
 *                   displacement noise (nullptr for none).
 * @param  bbox      Bounding box of the generated region in (xmin, xmax, ymin,
 *                   ymax) order.
 *
 * @return           A 2D Array object containing the generated conical
 *                   heightmap.
 *
 * **Example**
 * @include ex_cone.cpp
 *
 * **Result**
 * @image html ex_cone.png
 * */
Array cone_sigmoid(Vec2<int>    shape,
                   float        alpha,
                   float        radius = 0.5f,
                   Vec2<float>  center = {0.5f, 0.5f},
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a constant value array.
 *
 * @param  shape Array shape.
 * @param  value Filling value.
 * @return       Array New array.
 */
Array constant(Vec2<int> shape, float value = 0.f);

/**
 * @brief Return a crater-shaped heightmap.
 *
 * @param  shape                Array shape.
 * @param  radius               Crater radius.
 * @param  lip_decay            Ejecta lip decay.
 * @param  lip_height_ratio     Controls the ejecta lip relative height, in [0,
 *                              1].
 * @param  depth                Crater depth.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the lip_height_ratio
 *                              parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_crater.cpp
 *
 * **Result**
 * @image html ex_crater.png
 */
Array crater(Vec2<int>    shape,
             float        radius,
             float        depth,
             float        lip_decay,
             float        lip_height_ratio = 0.5f,
             const Array *p_ctrl_param = nullptr,
             const Array *p_noise_x = nullptr,
             const Array *p_noise_y = nullptr,
             Vec2<float>  center = {0.5f, 0.5f},
             Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a cubic pulse array.
 */
Array cubic_pulse(Vec2<int>    shape,
                  const Array *p_noise_x,
                  const Array *p_noise_y,
                  Vec2<float>  center = {0.5f, 0.5f},
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Dendry is a locally computable procedural function that generates
 * branching patterns at various scales (see @cite Gaillard2019).
 *
 * @param  shape                       Array shape.
 * @param  kw                          Noise wavenumber with respect to a unit
 *                                     domain.
 * @param  seed                        Random seed number.
 * @param  control_array               Control array (can be of any shape,
 *                                     different from
 * `shape`).
 * @param  eps                         Epsilon used to bias the area where
 *                                     points are generated in cells.
 * @param  resolution                  Number of resolutions in the noise
 *                                     function.
 * @param  displacement                Maximum displacement of segments.
 * @param  primitives_resolution_steps Additional resolution steps in the
 *                                     ComputeColorPrimitives function.
 * @param  slope_power                 Additional parameter to control the
 *                                     variation of slope on terrains.
 * @param  noise_amplitude_proportion  Proportion of the amplitude of the
 *                                     control function as noise.
 * @param  add_control_function        Add control function to the output.
 * @param  control_function_overlap    Extent of the extension added at the
 *                                     domain frontiers of the control array.
 * @param  p_noise_x, p_noise_y        Reference to the input noise arrays.
 * @param  p_stretching                Local wavenumber multiplier.
 * @param  bbox                        Domain bounding box.
 * @return                             Array New array.
 *
 * **Example**
 * @include ex_dendry.cpp
 *
 * **Result**
 * @image html ex_dendry.png
 */
Array dendry(Vec2<int>    shape,
             Vec2<float>  kw,
             uint         seed,
             Array       &control_function,
             float        eps = 0.05,
             int          resolution = 1,
             float        displacement = 0.075,
             int          primitives_resolution_steps = 3,
             float        slope_power = 2.f,
             float        noise_amplitude_proportion = 0.01,
             bool         add_control_function = true,
             float        control_function_overlap = 0.5f,
             const Array *p_noise_x = nullptr,
             const Array *p_noise_y = nullptr,
             const Array *p_stretching = nullptr,
             Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f},
             int          subsampling = 1);

Array dendry(Vec2<int>      shape,
             Vec2<float>    kw,
             uint           seed,
             NoiseFunction &noise_function,
             float          noise_function_offset = 0.f,
             float          noise_function_scaling = 1.f,
             float          eps = 0.05,
             int            resolution = 1,
             float          displacement = 0.075,
             int            primitives_resolution_steps = 3,
             float          slope_power = 2.f,
             float          noise_amplitude_proportion = 0.01,
             bool           add_control_function = true,
             float          control_function_overlap = 0.5f,
             const Array   *p_noise_x = nullptr,
             const Array   *p_noise_y = nullptr,
             const Array   *p_stretching = nullptr,
             Vec4<float>    bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a diffusion-limited aggregation (DLA) pattern.
 *
 * This function simulates the diffusion-limited aggregation process to generate
 * a pattern within a grid of specified dimensions. The DLA process models
 * particles that undergo a random walk until they stick to a seed, gradually
 * forming complex fractal structures.
 *
 * @param  shape                      The dimensions of the grid where the DLA
 *                                    pattern will be generated. It is
 *                                    represented as a `Vec2<int>` object, where
 *                                    the first element is the width and the
 *                                    second element is the height.
 * @param  scale                      A scaling factor that influences the
 *                                    density of the particles in the DLA
 *                                    pattern.
 * @param  seeding_radius             The radius within which initial seeding of
 *                                    particles occurs. This radius defines the
 *                                    area where the first particles are placed.
 * @param  seeding_outer_radius_ratio The ratio between the outer seeding radius
 *                                    and the initial seeding radius. It
 *                                    determines the outer boundary for particle
 *                                    seeding.
 * @param  slope                      Slope of the talus added to the DLA
 *                                    pattern.
 * @param  noise_ratio                A parameter that controls the amount of
 *                                    randomness or noise introduced in the
 *                                    talus formation process.
 * @param  seed                       The seed for the random number generator,
 *                                    ensuring reproducibility of the pattern.
 *                                    The same seed will generate the same
 *                                    pattern.
 *
 * @return                            A 2D array representing the generated DLA
 *                                    pattern. The array is of the same size as
 *                                    specified by `shape`.
 *
 * **Example**
 * @include ex_diffusion_limited_aggregation.cpp
 *
 * **Result**
 * @image html ex_diffusion_limited_aggregation.png
 */
Array diffusion_limited_aggregation(Vec2<int> shape,
                                    float     scale,
                                    uint      seed,
                                    float     seeding_radius = 0.4f,
                                    float     seeding_outer_radius_ratio = 0.2f,
                                    float     slope = 8.f,
                                    float     noise_ratio = 0.2f);

/**
 * @brief Generates a disk-shaped heightmap with optional modifications.
 *
 * This function creates a 2D array representing a disk shape with a specified
 * radius, slope, and other optional parameters such as control parameters,
 * noise, and stretching for additional customization.
 *
 * @param  shape        Dimensions of the output array (width, height).
 * @param  radius       Radius of the disk, in normalized coordinates (0.0 to
 *                      1.0).
 * @param  slope        Slope of the disk edge transition. A larger value makes
 *                      the edge transition sharper. Defaults to 1.0.
 * @param  p_ctrl_param Optional pointer to an `Array` controlling custom
 *                      parameters for the disk generation.
 * @param  p_noise_x    Optional pointer to an `Array` for adding noise in the
 *                      x-direction.
 * @param  p_noise_y    Optional pointer to an `Array` for adding noise in the
 *                      y-direction.
 * @param  p_stretching Optional pointer to an `Array` for stretching the disk
 *                      horizontally or vertically.
 * @param  center       Center of the disk in normalized coordinates (0.0 to
 *                      1.0). Defaults to {0.5, 0.5}.
 * @param  bbox         Bounding box for the disk in normalized coordinates
 *                     {x_min, x_max, y_min, y_max}. Defaults to {0.0, 1.0, 0.0,
 * 1.0}.
 *
 * @return              A 2D array representing the generated disk shape.
 *
 * * **Example**
 * @include ex_disk.cpp
 *
 * **Result**
 * @image html ex_disk.png
 */
Array disk(Vec2<int>    shape,
           float        radius,
           float        slope = 1.f,
           const Array *p_ctrl_param = nullptr,
           const Array *p_noise_x = nullptr,
           const Array *p_noise_y = nullptr,
           const Array *p_stretching = nullptr,
           Vec2<float>  center = {0.5f, 0.5f},
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a sparse Gabor noise.
 *
 * @param  shape   Array shape.
 * @param  kw      Kernel wavenumber, with respect to a unit domain.
 * @param  angle   Kernel angle (in degree).
 * @param  width   Kernel width (in pixels).
 * @param  density Spot noise density.
 * @param  seed    Random seed number.
 * @return         Array New array.
 *
 * **Example**
 * @include ex_gabor_noise.cpp
 *
 * **Result**
 * @image html ex_gabor_noise.png
 */
Array gabor_noise(Vec2<int> shape,
                  float     kw,
                  float     angle,
                  int       width,
                  float     density,
                  uint      seed);

/**
 * @brief Return a gaussian_decay pulse kernel.
 *
 * @param  shape                Array shape.
 * @param  sigma                Gaussian sigma (in pixels).
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the half-width parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  center               Primitive reference center.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Array
 *
 * **Example**
 * @include ex_gaussian_pulse.cpp
 *
 * **Result**
 * @image html ex_gaussian_pulse.png
 */
Array gaussian_pulse(Vec2<int>    shape,
                     float        sigma,
                     const Array *p_ctrl_param = nullptr,
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     const Array *p_stretching = nullptr,
                     Vec2<float>  center = {0.5f, 0.5f},
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a 2D island mask by perturbing a radial boundary with noise.
 *
 * Creates a binary mask of a single connected island using radial distance from
 * a center point and adding an FBM-based angular displacement.
 *
 * @param  shape        Output array size (width, height).
 * @param  radius       Base island radius.
 * @param  seed         Noise seed.
 * @param  displacement Amplitude of boundary perturbation.
 * @param  noise_type   Type of underlying noise function.
 * @param  kw           Angular frequency for sampling the noise.
 * @param  octaves      Number of FBM octaves.
 * @param  weight       Base amplitude of the FBM.
 * @param  persistence  Amplitude falloff per octave.
 * @param  lacunarity   Frequency multiplier per octave.
 * @param  center       Center of the island in normalized coordinates.
 * @param  bbox         Bounding box for coordinate remapping.
 *
 * @return              Binary mask where 1.f indicates land and 0.f indicates
 *                      water.
 *
 * **Example**
 * @include ex_island.cpp
 *
 * **Result**
 * @image html ex_island.png
 */
Array island_land_mask(Vec2<int>          shape,
                       float              radius,
                       uint               seed,
                       float              displacement = 0.2f,
                       NoiseType          noise_type = NoiseType::SIMPLEX2S,
                       float              kw = 4.f,
                       int                octaves = 8,
                       float              weight = 0.f,
                       float              persistence = 0.5f,
                       float              lacunarity = 2.f,
                       const Vec2<float> &center = {0.5f, 0.5f},
                       const Vec4<float> &bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generate a multi-step height profile along a rotated axis.
 *
 * Constructs an Array where values follow a sequence of geometric steps blended
 * with an optional linear transition. Steps are rotated by the given angle, can
 * be shaped with an exponent, and may be modulated by optional control and
 * noise fields.
 *
 * @param  shape              Output array resolution.
 * @param  angle              Rotation angle in degrees.
 * @param  r                  Geometric ratio between successive step widths.
 * @param  nsteps             Number of steps.
 * @param  elevation_exponent Exponent shaping step heights.
 * @param  shape_gain         Gain used to reshape intra-step transitions.
 * @param  scale              Scale of the step axis.
 * @param  outer_slope        Slope outside the [0,1] axis interval.
 * @param  p_ctrl_param       Optional control parameter array.
 * @param  p_noise_x          Optional x-axis noise array.
 * @param  p_noise_y          Optional y-axis noise array.
 * @param  center             Center of the step axis.
 * @param  bbox               Bounding box of the domain.
 * @return                    Generated Array.
 *
 * **Example**
 * @include ex_multisteps.cpp
 *
 * **Result**
 * @image html ex_multisteps.png
 */
Array multisteps(Vec2<int>          shape,
                 float              angle,
                 float              r = 1.2f,
                 int                nsteps = 8,
                 float              elevation_exponent = 0.7f,
                 float              shape_gain = 4.f,
                 float              scale = 0.5f,
                 float              outer_slope = 0.1f,
                 const Array       *p_ctrl_param = nullptr,
                 const Array       *p_noise_x = nullptr,
                 const Array       *p_noise_y = nullptr,
                 const Vec2<float> &center = {0.5f, 0.5f},
                 const Vec4<float> &bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Resulting array.
 *
 * **Example**
 * @include ex_noise.cpp
 *
 * **Result**
 * @image html ex_noise.png
 */
Array noise(NoiseType    noise_type,
            Vec2<int>    shape,
            Vec2<float>  kw,
            uint         seed,
            const Array *p_noise_x = nullptr,
            const Array *p_noise_y = nullptr,
            const Array *p_stretching = nullptr,
            Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_fbm(NoiseType    noise_type,
                Vec2<int>    shape,
                Vec2<float>  kw,
                uint         seed,
                int          octaves = 8,
                float        weight = 0.7f,
                float        persistence = 0.5f,
                float        lacunarity = 2.f,
                const Array *p_ctrl_param = nullptr,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                const Array *p_stretching = nullptr,
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  gradient_scale       Gradient scale.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_iq(NoiseType    noise_type,
               Vec2<int>    shape,
               Vec2<float>  kw,
               uint         seed,
               int          octaves = 8,
               float        weight = 0.7f,
               float        persistence = 0.5f,
               float        lacunarity = 2.f,
               float        gradient_scale = 0.05f,
               const Array *p_ctrl_param = nullptr,
               const Array *p_noise_x = nullptr,
               const Array *p_noise_y = nullptr,
               const Array *p_stretching = nullptr,
               Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  warp0                Initial warp scale.
 * @param  damp0                Initial damp scale.
 * @param  warp_scale           Warp scale.
 * @param  damp_scale           Damp scale.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_jordan(NoiseType    noise_type,
                   Vec2<int>    shape,
                   Vec2<float>  kw,
                   uint         seed,
                   int          octaves = 8,
                   float        weight = 0.7f,
                   float        persistence = 0.5f,
                   float        lacunarity = 2.f,
                   float        warp0 = 0.4f,
                   float        damp0 = 1.f,
                   float        warp_scale = 0.4f,
                   float        damp_scale = 1.f,
                   const Array *p_ctrl_param = nullptr,
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   const Array *p_stretching = nullptr,
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherent fbm Parberry variant of Perlin
 * noise.
 *
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  mu                   Gradient magnitude exponent.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_parberry(Vec2<int>    shape,
                     Vec2<float>  kw,
                     uint         seed,
                     int          octaves = 8,
                     float        weight = 0.7f,
                     float        persistence = 0.5f,
                     float        lacunarity = 2.f,
                     float        mu = 1.02f,
                     const Array *p_ctrl_param = nullptr,
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     const Array *p_stretching = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm pingpong noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_pingpong(NoiseType    noise_type,
                     Vec2<int>    shape,
                     Vec2<float>  kw,
                     uint         seed,
                     int          octaves = 8,
                     float        weight = 0.7f,
                     float        persistence = 0.5f,
                     float        lacunarity = 2.f,
                     const Array *p_ctrl_param = nullptr,
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     const Array *p_stretching = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm ridged noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  k_smoothing          Smoothing parameter.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_ridged(NoiseType    noise_type,
                   Vec2<int>    shape,
                   Vec2<float>  kw,
                   uint         seed,
                   int          octaves = 8,
                   float        weight = 0.7f,
                   float        persistence = 0.5f,
                   float        lacunarity = 2.f,
                   float        k_smoothing = 0.1f,
                   const Array *p_ctrl_param = nullptr,
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   const Array *p_stretching = nullptr,
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence fbm swiss noise.
 *
 * @param  noise_type           Noise type.
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  warp_scale           Warp scale.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * **Example**
 * @include ex_noise_fbm.cpp
 *
 * **Result**
 * @image html ex_noise_fbm0.png
 * @image html ex_noise_fbm1.png
 * @image html ex_noise_fbm2.png
 * @image html ex_noise_fbm3.png
 * @image html ex_noise_fbm4.png
 * @image html ex_noise_fbm5.png
 * @image html ex_noise_fbm6.png
 */
Array noise_swiss(NoiseType    noise_type,
                  Vec2<int>    shape,
                  Vec2<float>  kw,
                  uint         seed,
                  int          octaves = 8,
                  float        weight = 0.7f,
                  float        persistence = 0.5f,
                  float        lacunarity = 2.f,
                  float        warp_scale = 0.1f,
                  const Array *p_ctrl_param = nullptr,
                  const Array *p_noise_x = nullptr,
                  const Array *p_noise_y = nullptr,
                  const Array *p_stretching = nullptr,
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a paraboloid.
 * @param  shape                Array shape.
 * @param  angle                Rotation angle.
 * @param  a                    Curvature parameter, first principal axis.
 * @param  b                    Curvature parameter, second principal axis.
 * @param  v0                   Value at the paraboloid center.
 * @param  reverse_x            Reverse coefficient of first principal axis.
 * @param  reverse_y            Reverse coefficient of second principal axis.
 * @param  p_base_elevation     Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Output array.
 *
 * **Example**
 * @include ex_paraboloid.cpp
 *
 * **Result**
 * @image html ex_paraboloid.png
 */
Array paraboloid(Vec2<int>    shape,
                 float        angle,
                 float        a,
                 float        b,
                 float        v0 = 0.f,
                 bool         reverse_x = false,
                 bool         reverse_y = false,
                 const Array *p_noise_x = nullptr,
                 const Array *p_noise_y = nullptr,
                 const Array *p_stretching = nullptr,
                 Vec2<float>  center = {0.5f, 0.5f},
                 Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a peak-shaped heightmap.
 *
 * @param  shape         Array shape.
 * @param  radius        Peak outer radius.
 * @param  p_noise       Reference to the input noise array used for domain
 *                       warping (NOT in pixels, with respect to a unit domain).
 * @param  noise_amp_r   Radial noise absolute scale (in pixels).
 * @param  noise_ratio_z Vertical noise relative scale (in [0, 1]).
 * @param  bbox          Domain bounding box.
 * @return               Array Resulting array.
 *
 * **Example**
 * @include ex_peak.cpp
 *
 * **Result**
 * @image html ex_peak.png
 */
Array peak(Vec2<int>    shape,
           float        radius,
           const Array *p_noise,
           float        noise_r_amp,
           float        noise_z_ratio,
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a phasor noise field based on a Gabor noise model and phase
 * profile.
 *
 * This function creates a phasor noise array using a Gabor kernel approach,
 * applying a specified phase profile, smoothing, and density settings. The
 * output is influenced by the shape, frequency, and various noise
 * characteristics, allowing fine control over the generated noise field.
 *
 * @param  phasor_profile     The phase profile to apply. Determines the type of
 *                            phasor function used (e.g., bulky cosine, peaky
 *                            cosine).
 * @param  shape              The dimensions of the output array as a 2D vector
 *                            (width x height).
 * @param  kw                 The wave number (frequency) of the Gabor kernel.
 * @param  angle              An array specifying the angle field for the Gabor
 *                            kernel orientation.
 * @param  seed               A seed value for the random number generator used
 *                            to create jittered spawn points for Gabor kernels.
 * @param  profile_delta      A parameter for adjusting the delta in the phase
 *                            profile function.
 * @param  density_factor     A scaling factor for the density of Gabor kernel
 *                            spawn points.
 * @param  kernel_width_ratio The ratio of the kernel width to the phase field
 *                            resolution.
 * @param  phase_smoothing    A factor for controlling the blending of the phase
 *                            profile. Larger values result in smoother
 *                            transitions.
 * @return                    An `Array` containing the generated phasor noise
 *                            field.
 *
 * @throws std::invalid_argumentIfaninvalid`phasor_profile`isprovided.
 *
 * @note If the kernel width is too small (less than 4), the function returns a
 * zeroed array.
 *
 * @details The function performs the following steps:
 * - Generates Gabor kernel spawn points using jittered random sampling.
 * - Constructs Gabor kernels based on the input angle field and applies them to
 * noise arrays.
 * - Computes a phase field from the Gabor noise using `atan2`.
 * - Applies the specified phase profile using the
 * `get_phasor_profile_function`.
 * - Smooths the phase field if `phase_smoothing` is greater than zero.
 *
 * **Example**
 * @include ex_phasor.cpp
 *
 * **Result**
 * @image html ex_phasor.png
 */
Array phasor(PhasorProfile phasor_profile,
             Vec2<int>     shape,
             float         kw,
             const Array  &angle,
             uint          seed,
             float         profile_delta = 0.1f,
             float         density_factor = 1.f,
             float         kernel_width_ratio = 2.f,
             float         phase_smoothing = 2.f);

/**
 * @brief Generates a fractal Brownian motion (fBm) noise field using layered
 * phasor profiles.
 *
 * @param  phasor_profile     The phase profile to apply for each noise layer
 *                            (e.g., bulky cosine, peaky cosine).
 * @param  shape              The dimensions of the output array as a 2D vector
 *                            (width x height).
 * @param  kw                 The base wave number (frequency) for the first
 *                            noise layer.
 * @param  angle              An array specifying the angle field for the Gabor
 *                            kernel orientation in each layer.
 * @param  seed               A seed value for the random number generator used
 *                            in all noise layers.
 * @param  profile_delta      A parameter for adjusting the delta in the phase
 *                            profile function.
 * @param  density_factor     A scaling factor for the density of Gabor kernel
 *                            spawn points in each layer.
 * @param  kernel_width_ratio The ratio of the kernel width to the phase field
 *                            resolution.
 * @param  phase_smoothing    A factor for controlling the blending of the phase
 *                            profile. Larger values result in smoother
 *                            transitions.
 * @param  octaves            The number of noise layers (octaves) to generate.
 * @param  weight             A factor for controlling amplitude adjustments
 *                            based on the previous layer's values.
 * @param  persistence        A factor controlling how amplitude decreases
 *                            across successive octaves. Values <1 cause rapid
 *                            decay.
 * @param  lacunarity         A factor controlling how frequency increases
 *                            across successive octaves. Values >1 cause rapid
 *                            growth.
 * @return                    An `Array` containing the generated fBm noise
 *                            field.
 *
 * @throws std::invalid_argumentIfaninvalid`phasor_profile`isprovidedtothe
 *                            underlying `phasor` function.
 *
 * **Example**
 * @include ex_phasor.cpp
 *
 * **Result**
 * @image html ex_phasor.png
 */
Array phasor_fbm(PhasorProfile phasor_profile,
                 Vec2<int>     shape,
                 float         kw,
                 const Array  &angle,
                 uint          seed,
                 float         profile_delta = 0.1f,
                 float         density_factor = 1.f,
                 float         kernel_width_ratio = 2.f,
                 float         phase_smoothing = 2.f,
                 int           octaves = 8,
                 float         weight = 0.7f,
                 float         persistence = 0.5f,
                 float         lacunarity = 2.f);

/**
 * @brief Generates a rectangle-shaped heightmap with optional modifications.
 *
 * This function creates a 2D array representing a rectangle with specified
 * dimensions, rotation, and optional parameters for customization such as
 * control parameters, noise, and stretching.
 *
 * @param  shape        Dimensions of the output array (width, height).
 * @param  rx           Half-width of the rectangle, in normalized coordinates
 *                      (0.0 to 1.0).
 * @param  ry           Half-height of the rectangle, in normalized coordinates
 *                      (0.0 to 1.0).
 * @param  angle        Rotation angle of the rectangle in radians. Positive
 *                      values rotate counterclockwise.
 * @param  slope        Slope of the rectangle edge transition. A larger value
 *                      makes the edge transition sharper. Defaults to 1.0.
 * @param  p_ctrl_param Optional pointer to an `Array` controlling custom
 *                      parameters for the rectangle generation.
 * @param  p_noise_x    Optional pointer to an `Array` for adding noise in the
 *                      x-direction.
 * @param  p_noise_y    Optional pointer to an `Array` for adding noise in the
 *                      y-direction.
 * @param  p_stretching Optional pointer to an `Array` for stretching the
 *                      rectangle horizontally or vertically.
 * @param  center       Center of the rectangle in normalized coordinates (0.0
 *                      to 1.0). Defaults to {0.5, 0.5}.
 * @param  bbox         Bounding box for the rectangle in normalized coordinates
 *                     {x_min, x_max, y_min, y_max}. Defaults to {0.0, 1.0, 0.0,
 * 1.0}.
 *
 * @return              A 2D array representing the generated rectangle shape.
 *    *
 * **Example**
 * @include ex_rectangle.cpp
 *
 * **Result**
 * @image html ex_rectangle.png
 */
Array rectangle(Vec2<int>    shape,
                float        rx,
                float        ry,
                float        angle,
                float        slope = 1.f,
                const Array *p_ctrl_param = nullptr,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                const Array *p_stretching = nullptr,
                Vec2<float>  center = {0.5f, 0.5f},
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a rift function (Heaviside with an optional talus slope at the
 * transition).
 *
 * @param  shape                Array shape.
 * @param  angle                Overall rotation angle (in degree).
 * @param  slope                Step slope (assuming a unit domain).
 * @param  width                Rift width (assuming a unit domain).
 * @param  sharp_bottom         Decide whether the rift bottom is sharp or not.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the width parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local coordinate multiplier.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_rift.cpp
 *
 * **Result**
 * @image html ex_rift.png
 */
Array rift(Vec2<int>    shape,
           float        angle,
           float        slope,
           float        width,
           bool         sharp_bottom = false,
           const Array *p_ctrl_param = nullptr,
           const Array *p_noise_x = nullptr,
           const Array *p_noise_y = nullptr,
           const Array *p_stretching = nullptr,
           Vec2<float>  center = {0.5f, 0.5f},
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array corresponding to a slope with a given overall.
 *
 * @param  shape                Array shape.
 * @param  angle                Overall rotation angle (in degree).
 * @param  slope                Slope (assuming a unit domain).
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the slope parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local coordinate multiplier.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_slope.cpp
 *
 * **Result**
 * @image html ex_slope.png
 */
Array slope(Vec2<int>    shape,
            float        angle,
            float        slope,
            const Array *p_ctrl_param = nullptr,
            const Array *p_noise_x = nullptr,
            const Array *p_noise_y = nullptr,
            const Array *p_stretching = nullptr,
            Vec2<float>  center = {0.5f, 0.5f},
            Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a smooth cosine array.
 */
Array smooth_cosine(Vec2<int>    shape,
                    const Array *p_noise_x,
                    const Array *p_noise_y,
                    Vec2<float>  center = {0.5f, 0.5f},
                    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a step function (Heaviside with an optional talus slope at the
 * transition).
 *
 * @param  shape                Array shape.
 * @param  angle                Overall rotation angle (in degree).
 * @param  slope                Step slope (assuming a unit domain).
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the slope parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local coordinate multiplier.
 * @param  center               Primitive reference center.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_step.cpp
 *
 * **Result**
 * @image html ex_step.png
 */
Array step(Vec2<int>    shape,
           float        angle,
           float        slope,
           const Array *p_ctrl_param = nullptr,
           const Array *p_noise_x = nullptr,
           const Array *p_noise_y = nullptr,
           const Array *p_stretching = nullptr,
           Vec2<float>  center = {0.5f, 0.5f},
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generate displacements `dx` and `dy` to apply a swirl effect to
 * another primitve.
 *
 * @param dx[out]   'x' displacement (unit domain scale).
 * @param dy[out]   'y' displacement (unit domain scale).
 * @param amplitude Displacement amplitude.
 * @param exponent  Distance exponent.
 * @param p_noise   eference to the input noise array.
 * @param bbox      Domain bounding box.
 *
 * **Example**
 * @include ex_swirl.cpp
 *
 * **Result**
 * @image html ex_swirl.png
 */
void swirl(Array       &dx,
           Array       &dy,
           float        amplitude = 1.f,
           float        exponent = 1.f,
           const Array *p_noise = nullptr,
           Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a dune shape wave.
 *
 * @param  shape                Array shape.
 * @param  kw                   Wavenumber with respect to a unit domain.
 * @param  angle                Overall rotation angle (in degree).
 * @param  xtop                 Relative location of the top of the dune profile
 *                              (in [0, 1]).
 * @param  xbottom              Relative location of the foot of the dune
 *                              profile (in [0, 1]).
 * @param  phase_shift          Phase shift (in radians).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 */
Array wave_dune(Vec2<int>    shape,
                float        kw,
                float        angle,
                float        xtop,
                float        xbottom,
                float        phase_shift = 0.f,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                const Array *p_stretching = nullptr,
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a sine wave.
 *
 * @param  shape                Array shape.
 * @param  kw                   Wavenumber with respect to a unit domain.
 * @param  angle                Overall rotation angle (in degree).
 * @param  phase_shift          Phase shift (in radians).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_wave.cpp
 *
 * **Result**
 * @image html ex_wave0.png
 * @image html ex_wave1.png
 */
Array wave_sine(Vec2<int>    shape,
                float        kw,
                float        angle,
                float        phase_shift = 0.f,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                const Array *p_stretching = nullptr,
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a square wave.
 *
 * @param  shape                Array shape.
 * @param  kw                   Wavenumber with respect to a unit domain.
 * @param  angle                Overall rotation angle (in degree).
 * @param  phase_shift          Phase shift (in radians).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_wave.cpp
 *
 * **Result**
 * @image html ex_wave0.png
 * @image html ex_wave1.png
 */
Array wave_square(Vec2<int>    shape,
                  float        kw,
                  float        angle,
                  float        phase_shift = 0.f,
                  const Array *p_noise_x = nullptr,
                  const Array *p_noise_y = nullptr,
                  const Array *p_stretching = nullptr,
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return a triangular wave.
 *
 * @param  shape                Array shape.
 * @param  kw                   Wavenumber with respect to a unit domain.
 * @param  angle                Overall rotation angle (in degree).
 * @param  slant_ratio          Relative location of the triangle apex, in [0,
 *                              1].
 * @param  phase_shift          Phase shift (in radians).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array New array.
 *
 * **Example**
 * @include ex_wave.cpp
 *
 * **Result**
 * @image html ex_wave0.png
 * @image html ex_wave1.png
 */
Array wave_triangular(Vec2<int>    shape,
                      float        kw,
                      float        angle,
                      float        slant_ratio,
                      float        phase_shift = 0.f,
                      const Array *p_noise_x = nullptr,
                      const Array *p_noise_y = nullptr,
                      const Array *p_stretching = nullptr,
                      Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with white noise.
 *
 * @param  shape Array shape.
 * @param  a     Lower bound of random distribution.
 * @param  b     Upper bound of random distribution.
 * @param  seed  Random number seed.
 * @return       Array White noise.
 *
 * **Example**
 * @include ex_white.cpp
 *
 * **Result**
 * @image html ex_white.png
 *
 * @see          {@link white_sparse}
 */
Array white(Vec2<int> shape, float a, float b, uint seed);

/**
 * @brief Return an array filled `1` with a probability based on a density map.
 *
 * @param  density_map Density map.
 * @param  seed        Random number seed.
 * @return             Array New array.
 *
 * **Example**
 * @include ex_white_density_map.cpp
 *
 * **Result**
 * @image html ex_white_density_map.png
 */
Array white_density_map(const Array &density_map, uint seed);

/**
 * @brief Return an array sparsely filled with white noise.
 *
 * @param  shape   Array shape.
 * @param  a       Lower bound of random distribution.
 * @param  b       Upper bound of random distribution.
 * @param  density Array filling density, in [0, 1]. If set to 1, the function
 *                 is equivalent to {@link white}.
 * @param  seed    Random number seed.
 * @return         Array Sparse white noise.
 *
 * **Example**
 * @include ex_white_sparse.cpp
 *
 * **Result**
 * @image html ex_white_sparse.png
 *
 * @see            {@link white}
 */
Array white_sparse(Vec2<int> shape, float a, float b, float density, uint seed);

/**
 * @brief Return an array sparsely filled with random 0 and 1.
 *
 * @param  shape   Array shape.
 * @param  density Array filling density, in [0, 1]. If set to 1, the function
 *                 is equivalent to {@link white}.
 * @param  seed    Random number seed.
 * @return         Array Sparse white noise.
 */
Array white_sparse_binary(Vec2<int> shape, float density, uint seed);

/**
 * @brief Return an array filled with the maximum of two Worley (cellular)
 * noises.
 *
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions,
 *                              with respect to a unit domain.
 * @param  seed                 Random seed number.
 * @param  ratio                Amplitude ratio between each Worley noise.
 * @param  k                    Transition smoothing parameter.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the ratio parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  p_stretching         Local wavenumber multiplier.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Noise.
 *
 * **Example**
 * @include ex_worley_double.cpp
 *
 * **Result**
 * @image html ex_worley_double.png
 */
Array worley_double(Vec2<int>    shape,
                    Vec2<float>  kw,
                    uint         seed,
                    float        ratio = 0.5f,
                    float        k = 0.f,
                    const Array *p_ctrl_param = nullptr,
                    const Array *p_noise_x = nullptr,
                    const Array *p_noise_y = nullptr,
                    const Array *p_stretching = nullptr,
                    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

} // namespace hmap

namespace hmap::gpu
{

/**
 * @brief Generates a synthetic "badlands" terrain heightmap.
 *
 * This function procedurally creates a 2D elevation field resembling badlands
 * (highly eroded terrain with sharp ridges and valleys). It combines fractal
 * noise (FBM) with a Voronoi-based primitive, displaced along a specified
 * orientation.
 *
 * @param  shape          Output array shape (resolution in x and y).
 * @param  kw             Frequency vector controlling the scale of the
 *                        features.
 * @param  seed           Random seed used for noise and Voronoi generation.
 * @param  rugosity       Controls roughness of the fractal noise (higher = more
 *                        irregular).
 * @param  angle          Orientation angle (degrees) of terrain displacements.
 * @param  k_smoothing    Voronoi smoothing parameter (controls ridge
 *                        sharpness).
 * @param  base_noise_amp Amplitude of the base displacement noise.
 * @param  p_noise_x      Optional pointer to external displacement noise field
 *                        (X-axis).
 * @param  p_noise_y      Optional pointer to external displacement noise field
 *                        (Y-axis).
 * @param  bbox           Bounding box of the generation domain in normalized
 *                        coordinates.
 *
 * @return                Array containing the generated badlands heightmap.
 *
 * **Example**
 * @include ex_badlands.cpp
 *
 * **Result**
 * @image html ex_badlands.png
 */
Array badlands(Vec2<int>    shape,
               Vec2<float>  kw,
               uint         seed,
               int          octaves = 8,
               float        rugosity = 0.2f,
               float        angle = 30.f,
               float        k_smoothing = 0.1f,
               float        base_noise_amp = 0.2f,
               const Array *p_noise_x = nullptr,
               const Array *p_noise_y = nullptr,
               Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a synthetic procedural terrain resembling basaltic
 * landforms.
 *
 * This function creates a multi-scale procedural field combining large, medium,
 * and small-scale Voronoi-based patterns, noise warping, and optional
 * flattening, simulating the morphology of fractured basalt or volcanic
 * terrains. The terrain is constructed using a combination of Voronoi diagrams
 * (via `voronoi_fbm`) and fractal noise (`noise_fbm`), layered with
 * frequency-domain manipulations and amplitude/gain controls at each scale.
 *
 * The final output is a heightmap represented as an `Array`, normalized and
 * composed of:
 * - Large-scale cellular patterns with smoothed Voronoi edge distances.
 * - Medium and small-scale structures introducing finer surface variation.
 * - Optional rugosity (fine detail) and flattening to simulate erosion or flow
 * effects.
 *
 * @param  shape                   Output resolution (width x height) of the
 *                                 field.
 * @param  kw                      Base wave numbers (frequency) for the terrain
 *                                 features.
 * @param  seed                    Initial seed used for deterministic random
 *                                 generation.
 * @param  warp_kw                 Frequency of the warping noise that displaces
 *                                 Voronoi positions.
 * @param  large_scale_warp_amp    Amplitude of displacement for large-scale
 *                                 Voronoi warping.
 * @param  large_scale_gain        Gain adjustment applied to the large-scale
 *                                 features.
 * @param  large_scale_amp         Final amplitude of the large-scale height
 *                                 contribution.
 * @param  medium_scale_kw_ratio   Scaling factor for the frequency of the
 *                                 medium-scale patterns.
 * @param  medium_scale_warp_amp   Amplitude of warping for the medium-scale
 *                                 displacement.
 * @param  medium_scale_gain       Gain control for medium-scale modulation.
 * @param  medium_scale_amp        Amplitude of the medium-scale heightmap.
 * @param  small_scale_kw_ratio    Frequency ratio for small-scale details.
 * @param  small_scale_amp         Amplitude of small-scale pattern
 *                                 contribution.
 * @param  small_scale_overlay_amp Additional overlay strength for repeating the
 *                                 small-scale pattern.
 * @param  rugosity_kw_ratio       Frequency ratio for high-frequency noise
 *                                 applied as fine roughness.
 * @param  rugosity_amp            Strength of the rugosity (high-frequency
 *                                 modulation).
 * @param  flatten_activate        Enables or disables the final flattening
 *                                 operation.
 * @param  flatten_kw_ratio        Frequency scaling of the flattening noise
 *                                 field.
 * @param  flatten_amp             Amplitude control of the flattening
 *                                 operation.
 * @param  p_noise_x               Optional pointer to a noise field used to
 *                                 displace grid coordinates in X.
 * @param  p_noise_y               Optional pointer to a noise field used to
 *                                 displace grid coordinates in Y.
 * @param  bbox                    The 2D bounding box ({xmin, xmax, ymin,
 *                                 ymax}) over which the terrain is generated.
 *
 * @return                         A procedurally generated `Array` representing
 *                                 the synthetic basalt-like terrain field.
 *
 * @note
 * - This function relies on OpenCL-based kernels via the `gpu::` namespace.
 * - The returned field is normalized in amplitude but may require rescaling to
 * match specific physical units.
 * - Adjusting `seed`, `warp_kw`, and the gain/amplitude values can produce a
 * wide variety of terrain features.
 *
 * **Example**
 * @include ex_basalt_field.cpp
 *
 * **Result**
 * @image html ex_basalt_field.png
 */
Array basalt_field(Vec2<int>    shape,
                   Vec2<float>  kw,
                   uint         seed,
                   float        warp_kw = 4.f,
                   float        large_scale_warp_amp = 0.2f,
                   float        large_scale_gain = 6.f,
                   float        large_scale_amp = 0.2f,
                   float        medium_scale_kw_ratio = 3.f,
                   float        medium_scale_warp_amp = 1.f,
                   float        medium_scale_gain = 7.f,
                   float        medium_scale_amp = 0.08f,
                   float        small_scale_kw_ratio = 10.f,
                   float        small_scale_amp = 0.1f,
                   float        small_scale_overlay_amp = 0.002f,
                   float        rugosity_kw_ratio = 1.f,
                   float        rugosity_amp = 1.f,
                   bool         flatten_activate = true,
                   float        flatten_kw_ratio = 1.f,
                   float        flatten_amp = 0.f,
                   const Array *p_noise_x = nullptr,
                   const Array *p_noise_y = nullptr,
                   Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence Gabor noise.
 *
 * @param  shape              Array shape.
 * @param  kw                 Noise wavenumbers {kx, ky} for each directions.
 * @param  seed               Random seed number.
 * @param  angle              Base orientation angle for the Gabor wavelets (in
 *                            radians). Defaults to 0.
 * @param  angle_spread_ratio Ratio that controls the spread of wave
 *                            orientations around the base angle. Defaults to 1.
 * @param  bbox               Domain bounding box.
 * @return                    Array Fractal noise.
 *
 * @note Taken from https://www.shadertoy.com/view/clGyWm
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_gabor_wave.cpp
 *
 * **Result**
 * @image html ex_gabor_wave.png
 */
Array gabor_wave(Vec2<int>    shape,
                 Vec2<float>  kw,
                 uint         seed,
                 const Array &angle,
                 float        angle_spread_ratio = 1.f,
                 Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

Array gabor_wave(Vec2<int>   shape,
                 Vec2<float> kw,
                 uint        seed,
                 float       angle = 0.f,
                 float       angle_spread_ratio = 1.f,
                 Vec4<float> bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence Gabor noise.
 *
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  angle                Base orientation angle for the Gabor wavelets
 *                              (in radians). Defaults to 0.
 * @param  angle_spread_ratio   Ratio that controls the spread of wave
 *                              orientations around the base angle. Defaults to
 *                              1.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * @note Taken from https://www.shadertoy.com/view/clGyWm
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_gabor_wave.cpp
 *
 * **Result**
 * @image html ex_gabor_wave.png
 */
Array gabor_wave_fbm(Vec2<int>    shape,
                     Vec2<float>  kw,
                     uint         seed,
                     const Array &angle,
                     float        angle_spread_ratio = 1.f,
                     int          octaves = 8,
                     float        weight = 0.7f,
                     float        persistence = 0.5f,
                     float        lacunarity = 2.f,
                     const Array *p_ctrl_param = nullptr,
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

Array gabor_wave_fbm(Vec2<int>    shape,
                     Vec2<float>  kw,
                     uint         seed,
                     float        angle = 0.f,
                     float        angle_spread_ratio = 1.f,
                     int          octaves = 8,
                     float        weight = 0.7f,
                     float        persistence = 0.5f,
                     float        lacunarity = 2.f,
                     const Array *p_ctrl_param = nullptr,
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a 2D array using the GavoroNoise algorithm, which is a
 * procedural noise technique for terrain generation and other applications.
 *
 * @param  shape           Dimensions of the output array.
 * @param  kw              Wave number vector controlling the noise frequency.
 * @param  seed            Seed value for random number generation.
 * @param  amplitude       Amplitude of the noise.
 * @param  kw_multiplier   Multiplier for wave numbers in the noise function.
 * @param  slope_strength  Strength of slope-based directional erosion in the
 *                         noise.
 * @param  branch_strength Strength of branch-like structures in the generated
 *                         noise.
 * @param  z_cut_min       Minimum cutoff for Z-value in the noise.
 * @param  z_cut_max       Maximum cutoff for Z-value in the noise.
 * @param  octaves         Number of octaves for fractal Brownian motion (fBm).
 * @param  persistence     Amplitude scaling factor between noise octaves.
 * @param  lacunarity      Frequency scaling factor between noise octaves.
 * @param  p_ctrl_param    Optional array for control parameters, can modify the
 *                         Z cutoff dynamically.
 * @param  p_noise_x       Optional array for X-axis noise perturbation.
 * @param  p_noise_y       Optional array for Y-axis noise perturbation.
 * @param  bbox            Bounding box for mapping grid coordinates to world
 *                         space.
 *
 * @return                 A 2D array containing the generated GavoroNoise
 *                         values.
 *
 * @note Taken from https://www.shadertoy.com/view/MtGcWh
 *
 * @note Only available if OpenCL is enabled.
 *
 * This function leverages an OpenCL kernel to compute the GavoroNoise values on
 * the GPU, allowing for efficient large-scale generation. The kernel applies a
 * combination of fractal Brownian motion (fBm), directional erosion, and other
 * procedural techniques to generate intricate noise patterns.
 *
 * The optional `p_ctrl_param`, `p_noise_x`, and `p_noise_y` buffers provide
 * additional flexibility for dynamically adjusting noise parameters and
 * perturbations.
 *
 * **Example**
 * @include ex_gavoronoise.cpp
 *
 * **Result**
 * @image html ex_gavoronoise.png
 */
Array gavoronoise(Vec2<int>    shape,
                  Vec2<float>  kw,
                  uint         seed,
                  const Array &angle,
                  float        amplitude = 0.05f,
                  float        angle_spread_ratio = 1.f,
                  Vec2<float>  kw_multiplier = {4.f, 4.f},
                  float        slope_strength = 1.f,
                  float        branch_strength = 2.f,
                  float        z_cut_min = 0.2f,
                  float        z_cut_max = 1.f,
                  int          octaves = 8,
                  float        persistence = 0.4f,
                  float        lacunarity = 2.f,
                  const Array *p_ctrl_param = nullptr,
                  const Array *p_noise_x = nullptr,
                  const Array *p_noise_y = nullptr,
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

Array gavoronoise(Vec2<int>    shape,
                  Vec2<float>  kw,
                  uint         seed,
                  float        angle = 0.f,
                  float        amplitude = 0.05f,
                  float        angle_spread_ratio = 1.f,
                  Vec2<float>  kw_multiplier = {4.f, 4.f},
                  float        slope_strength = 1.f,
                  float        branch_strength = 2.f,
                  float        z_cut_min = 0.2f,
                  float        z_cut_max = 1.f,
                  int          octaves = 8,
                  float        persistence = 0.4f,
                  float        lacunarity = 2.f,
                  const Array *p_ctrl_param = nullptr,
                  const Array *p_noise_x = nullptr,
                  const Array *p_noise_y = nullptr,
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

Array gavoronoise(const Array &base,
                  Vec2<float>  kw,
                  uint         seed,
                  float        amplitude = 0.05f,
                  Vec2<float>  kw_multiplier = {4.f, 4.f},
                  float        slope_strength = 1.f,
                  float        branch_strength = 2.f,
                  float        z_cut_min = 0.2f,
                  float        z_cut_max = 1.f,
                  int          octaves = 8,
                  float        persistence = 0.4f,
                  float        lacunarity = 2.f,
                  const Array *p_ctrl_param = nullptr,
                  const Array *p_noise_x = nullptr,
                  const Array *p_noise_y = nullptr,
                  Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a scalar field representing the signed distance to randomly
 * generated hemispheres.
 *
 * @param  shape     Resolution of the output field (width, height).
 * @param  kw        Scaling factor for field coordinates (world units per
 *                   pixel).
 * @param  seed      Random seed generation.
 * @param  rmin      Minimum sphere radius (relative to bbox size).
 * @param  rmax      Maximum sphere radius (relative to bbox size).
 * @param  density   Fraction of pixels covered by polygons (approximate).
 * @param  jitter    Random displacement factor applied to sphere vertices.
 * @param  shift     Random position shift applied to sphere center.
 * @param  p_noise_x Optional displacement noise field in the X direction
 *                   (nullptr to disable).
 * @param  p_noise_y Optional displacement noise field in the Y direction
 *                   (nullptr to disable).
 * @param  bbox      Bounding box in normalized coordinates {xmin, xmax, ymin,
 *                   ymax}.
 * @return           Array         2D array containing the signed distance
 *                   field.
 *
 * **Example**
 * @include ex_phemisphere_field.cpp
 *
 * **Result**
 * @image html ex_phemisphere_field.png
 */
Array hemisphere_field(Vec2<int>         shape,
                       Vec2<float>       kw,
                       uint              seed,
                       float             rmin = 0.05f,
                       float             rmax = 0.8f,
                       float             amplitude_random_ratio = 1.f,
                       float             density = 0.1f,
                       hmap::Vec2<float> jitter = {1.f, 1.f},
                       float             shift = 0.f,
                       const Array      *p_noise_x = nullptr,
                       const Array      *p_noise_y = nullptr,
                       const Array      *p_noise_distance = nullptr,
                       const Array      *p_density_multiplier = nullptr,
                       const Array      *p_size_multiplier = nullptr,
                       Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/*! @brief See hmap::hemisphere_field */
Array hemisphere_field_fbm(Vec2<int>         shape,
                           Vec2<float>       kw,
                           uint              seed,
                           float             rmin = 0.05f,
                           float             rmax = 0.8f,
                           float             amplitude_random_ratio = 1.f,
                           float             density = 0.1f,
                           hmap::Vec2<float> jitter = {0.5f, 0.5f},
                           float             shift = 0.1f,
                           int               octaves = 8,
                           float             persistence = 0.5f,
                           float             lacunarity = 2.f,
                           const Array      *p_noise_x = nullptr,
                           const Array      *p_noise_y = nullptr,
                           const Array      *p_noise_distance = nullptr,
                           const Array      *p_density_multiplier = nullptr,
                           const Array      *p_size_multiplier = nullptr,
                           Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates an island heightmap from a land mask using radial profiles,
 * slope shaping, optional noise, and water attenuation.
 *
 * Applies elevation shaping inside the land mask using distance-based profiles,
 * slope controls, optional radial and slope noise, and water-depth modeling.
 *
 * @param  land_mask              Binary mask defining the island shape.
 * @param  p_noise_r              Optional radial noise field (same size as
 *                                mask).
 * @param  apex_elevation         Maximum elevation at island center.
 * @param  filter_distance        Enable smoothing of the distance transform.
 * @param  filter_ir              Radius of the distance filter.
 * @param  slope_min              Minimum slope.
 * @param  slope_max              Maximum slope.
 * @param  slope_start            Start of slope ramp (0–1).
 * @param  slope_end              End of slope ramp (0–1).
 * @param  slope_noise_intensity  Intensity of slope noise modulation.
 * @param  k_smooth               Smoothing factor for the final profile.
 * @param  radial_noise_intensity Intensity of radial displacement noise.
 * @param  radial_profile_gain    Exponent for radial falloff.
 * @param  water_decay            Transition sharpness to water level.
 * @param  water_depth            Water depth below the shoreline.
 * @param  lee_angle              Angle for lee-side (wind shadow) erosion.
 * @param  lee_amp                Amplitude of lee-side erosion.
 * @param  uplift_amp             Amplitude of uplift (orographic) effect.
 * @param  p_water_depth          Optional output: per-pixel water depth.
 * @param  p_inland_mask          Optional output: mask of inland pixels.
 *
 * @return                        Heightmap array representing the generated
 *                                island.
 *
 * **Example**
 * @include ex_island.cpp
 *
 * **Result**
 * @image html ex_island.png
 */
Array island(const Array &land_mask,
             const Array *p_noise_r = nullptr,
             float        apex_elevation = 1.f,
             bool         filter_distance = true,
             int          filter_ir = 32,
             float        slope_min = 0.1f,
             float        slope_max = 8.f,
             float        slope_start = 0.5f,
             float        slope_end = 1.f,
             float        slope_noise_intensity = 0.5f,
             float        k_smooth = 0.05f,
             float        radial_noise_intensity = 0.3f,
             float        radial_profile_gain = 2.f,
             float        water_decay = 0.05f,
             float        water_depth = 0.3f,
             float        lee_angle = 30.f,
             float        lee_amp = 0.f,
             float        uplift_amp = 0.f,
             Array       *p_water_depth = nullptr,
             Array       *p_inland_mask = nullptr);

/**
 * @brief Generates an island heightmap from a land mask using internally
 * generated FBM noise for radial perturbation and slope modulation.
 *
 * Same as the other overload but creates a noise field procedurally from a
 * seed, including frequency, octaves, orientation, and smoothing controls.
 *
 * @param  land_mask              Binary mask defining the island shape.
 * @param  seed                   Noise seed.
 * @param  noise_amp              Amplitude of generated noise.
 * @param  noise_kw               Noise frequencies (x, y).
 * @param  noise_octaves          Number of FBM octaves.
 * @param  noise_rugosity         Persistence-like roughness control.
 * @param  noise_angle            Rotation of the noise field (radians).
 * @param  noise_k_smoothing      Smoothing applied to the noise field.
 * @param  apex_elevation         Maximum elevation at island center.
 * @param  filter_distance        Enable smoothing of the distance transform.
 * @param  filter_ir              Radius of distance filter.
 * @param  slope_min              Minimum slope.
 * @param  slope_max              Maximum slope.
 * @param  slope_start            Start of slope ramp (0–1).
 * @param  slope_end              End of slope ramp (0–1).
 * @param  slope_noise_intensity  Intensity of slope noise modulation.
 * @param  k_smooth               Smoothing factor for final profile.
 * @param  radial_noise_intensity Intensity of radial displacement noise.
 * @param  radial_profile_gain    Exponent for radial falloff.
 * @param  water_decay            Transition sharpness to water level.
 * @param  water_depth            Water depth below the shoreline.
 * @param  lee_angle              Angle for lee-side erosion.
 * @param  lee_amp                Amplitude for lee-side erosion.
 * @param  uplift_amp             Amplitude of uplift (orographic) effect.
 * @param  p_water_depth          Optional output: per-pixel water depth.
 * @param  p_inland_mask          Optional output: mask of inland pixels.
 *
 * @return                        Heightmap array representing the generated
 *                                island.
 *
 * **Example**
 * @include ex_island.cpp
 *
 * **Result**
 * @image html ex_island.png
 */
Array island(const Array &land_mask,
             uint         seed,
             float        noise_amp = 0.07f,
             Vec2<float>  noise_kw = {4.f, 4.f},
             int          noise_octaves = 8,
             float        noise_rugosity = 0.7f,
             float        noise_angle = 45.f,
             float        noise_k_smoothing = 0.05f,
             float        apex_elevation = 1.f,
             bool         filter_distance = true,
             int          filter_ir = 32,
             float        slope_min = 0.1f,
             float        slope_max = 8.f,
             float        slope_start = 0.5f,
             float        slope_end = 1.f,
             float        slope_noise_intensity = 0.5f,
             float        k_smooth = 0.05f,
             float        radial_noise_intensity = 0.3f,
             float        radial_profile_gain = 2.f,
             float        water_decay = 0.05f,
             float        water_depth = 0.3f,
             float        lee_angle = 30.f,
             float        lee_amp = 0.f,
             float        uplift_amp = 0.f,
             Array       *p_water_depth = nullptr,
             Array       *p_inland_mask = nullptr);

/**
 * @brief Generates a procedural "mountain cone" heightmap using fractal noise
 * and Voronoi patterns.
 *
 * This function synthesizes a terrain-like array shaped as a cone with
 * noise-based ridges and surface roughness. The generation combines a
 * cone-shaped envelope (`cone_sigmoid`), a fractal base noise
 * (`gpu::noise_fbm`), and a Voronoi ridge structure (`voronoi_fbm`), producing
 * a mountain with controlled smoothness, angular distortion, and noise-based
 * displacements.
 *
 * @param  shape          The output array dimensions (width, height).
 * @param  seed           The random seed for noise generation.
 * @param  scale          Global scaling factor for the mountain size.
 * @param  octaves        Number of fractal noise octaves for both FBM and
 *                        Voronoi.
 * @param  peak_kw        Controls the base frequency (inverse wavelength) of
 *                        the peak noise.
 * @param  rugosity       Controls the amplitude decay across octaves (higher =
 *                        rougher surface).
 * @param  angle          Direction (in degrees) for the displacement noise
 *                        distortion.
 * @param  k_smoothing    Smoothing factor applied in Voronoi blending.
 * @param  gamma          Gamma correction exponent applied at the end.
 * @param  cone_alpha     Controls the cone envelope steepness (higher = sharper
 *                        peak).
 * @param  ridge_amp      Controls the amplitude of ridge enhancement in the
 *                        Voronoi term.
 * @param  base_noise_amp Amplitude multiplier for the displacement noise.
 * @param  center         The 2D position (in normalized coordinates) of the
 *                        mountain cone center.
 * @param  p_noise_x      Optional pointer to an external X displacement noise
 *                        field (can be nullptr).
 * @param  p_noise_y      Optional pointer to an external Y displacement noise
 *                        field (can be nullptr).
 * @param  bbox           The bounding box (min_x, min_y, max_x, max_y) defining
 *                        the spatial extent of the map.
 *
 * @return                An Array representing the final terrain heightmap in
 *                        normalized [0, 1] range.
 *
 * **Example**
 * @include ex_mountain_cone.cpp
 *
 * **Result**
 * @image html ex_mountain_cone.png
 */
Array mountain_cone(Vec2<int>    shape,
                    uint         seed,
                    float        scale = 1.f,
                    int          octaves = 8,
                    float        peak_kw = 4.f,
                    float        rugosity = 0.f,
                    float        angle = 45.f,
                    float        k_smoothing = 0.f,
                    float        gamma = 0.5f,
                    float        cone_alpha = 1.f,
                    float        ridge_amp = 0.4f,
                    float        base_noise_amp = 0.05f,
                    Vec2<float>  center = {0.5f, 0.5f},
                    const Array *p_noise_x = nullptr,
                    const Array *p_noise_y = nullptr,
                    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a synthetic mountain-like inselberg (isolated hill)
 * heightmap.
 *
 * This function procedurally creates a 2D elevation field resembling an
 * inselberg, using a combination of fractal noise (FBM), Gaussian pulses, and
 * Voronoi-based structures. It allows control over scale, shape, orientation,
 * rugosity, and optional geological-like effects such as bulk uplift and
 * deposition smoothing.
 *
 * @param  shape          Output array shape (resolution in x and y).
 * @param  seed           Random seed used for noise and Voronoi generation.
 * @param  scale          Global scaling factor for the inselberg width and
 *                        structure.
 * @param  rugosity       Controls roughness of the fractal noise (higher = more
 *                        irregular).
 * @param  angle          Orientation angle (degrees) for directional
 *                        displacements.
 * @param  gamma          Gamma correction factor applied to the final
 *                        heightmap.
 * @param  round_shape    If true, generates a symmetric round shape;
 *                        if false, adds directional displacement.
 * @param  add_deposition If true, applies a smoothing fill step simulating
 *                        sediment deposition.
 * @param  bulk_amp       Amplitude of bulk uplift applied to the base pulse (0
 *                        = none, >0 = raises and normalizes the feature).
 * @param  base_noise_amp Amplitude of the base displacement noise.
 * @param  k_smoothing    Voronoi smoothing parameter (controls ridge
 *                        sharpness).
 * @param  center         Center of the inselberg in normalized coordinates.
 * @param  p_noise_x      Optional pointer to external displacement noise field
 *                        (X-axis).
 * @param  p_noise_y      Optional pointer to external displacement noise field
 *                        (Y-axis).
 * @param  bbox           Bounding box of the generation domain in normalized
 *                        coordinates.
 *
 * @return                Array containing the generated inselberg heightmap.
 *
 * **Example**
 * @include ex_mountain_inselberg.cpp
 *
 **Result**
 * @image html ex_mountain_inselberg.png
 */
Array mountain_inselberg(Vec2<int>    shape,
                         uint         seed,
                         float        scale = 1.f,
                         int          octaves = 8,
                         float        rugosity = 0.2f,
                         float        angle = 45.f,
                         float        gamma = 1.1f,
                         bool         round_shape = false,
                         bool         add_deposition = true,
                         float        bulk_amp = 0.2f,
                         float        base_noise_amp = 0.2f,
                         float        k_smoothing = 0.1f,
                         Vec2<float>  center = {0.5f, 0.5f},
                         const Array *p_noise_x = nullptr,
                         const Array *p_noise_y = nullptr,
                         Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a heightmap representing a radial mountain range.
 *
 * This function creates a heightmap that simulates a mountain range emanating
 * radially from a specified center. The mountain range is influenced by various
 * noise parameters and control attributes.
 *
 * @param  shape              The dimensions of the output heightmap as a 2D
 *                            vector.
 * @param  kw                 The wave numbers (frequency components) as a 2D
 *                            vector.
 * @param  seed               The seed for random noise generation.
 * @param  half_width         The half-width of the radial mountain range,
 *                            controlling its spread. Default is 0.2f.
 * @param  angle_spread_ratio The ratio controlling the angular spread of the
 *                            mountain range. Default is 0.5f.
 * @param  center             The center point of the radial mountain range as
 *                            normalized coordinates within [0, 1]. Default is
 *                           {0.5f, 0.5f}.
 * @param  octaves            The number of octaves for fractal noise
 *                            generation. Default is 8.
 * @param  weight             The initial weight for noise contribution. Default
 *                            is 0.7f.
 * @param  persistence        The amplitude scaling factor for subsequent noise
 *                            octaves. Default is 0.5f.
 * @param  lacunarity         The frequency scaling factor for subsequent noise
 *                            octaves. Default is 2.0f.
 * @param  p_ctrl_param       Optional pointer to an array of control parameters
 *                            influencing the terrain generation.
 * @param  p_noise_x          Optional pointer to a precomputed noise array for
 *                            the X-axis.
 * @param  p_noise_y          Optional pointer to a precomputed noise array for
 *                            the Y-axis.
 * @param  p_angle            Optional pointer to an array to output the angle.
 * @param  bbox               The bounding box of the output heightmap in
 *                            normalized coordinates [xmin, xmax, ymin, ymax].
 *                            Default is {0.0f, 1.0f, 0.0f, 1.0f}.
 *
 * @return                    Array The generated heightmap representing the
 *                            radial mountain range.
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_mountain_range_radial.cpp
 *
 * **Result**
 * @image html ex_mountain_range_radial.png
 */
Array mountain_range_radial(Vec2<int>    shape,
                            Vec2<float>  kw,
                            uint         seed,
                            float        half_width = 0.2f,
                            float        angle_spread_ratio = 0.5f,
                            float        core_size_ratio = 1.f,
                            Vec2<float>  center = {0.5f, 0.5f},
                            int          octaves = 8,
                            float        weight = 0.7f,
                            float        persistence = 0.5f,
                            float        lacunarity = 2.f,
                            const Array *p_ctrl_param = nullptr,
                            const Array *p_noise_x = nullptr,
                            const Array *p_noise_y = nullptr,
                            const Array *p_angle = nullptr,
                            Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a mountain-like heightmap with a flattened (stump-shaped)
 * peak.
 *
 * This function procedurally creates a terrain resembling a broad mountain with
 * a smooth, truncated summit. The result combines layered fractal noise (FBM),
 * Gaussian shaping, and Voronoi-based ridge formation to produce
 * natural-looking geomorphological features. The shape can be modulated by
 * directional displacement noise, smoothed using deposition simulation, and
 * refined through gamma and smoothing parameters.
 *
 * @param  shape          Dimensions of the output heightmap (width, height).
 * @param  seed           Random seed for noise generation.
 * @param  scale          Global scale factor controlling the size of features.
 * @param  octaves        Number of noise octaves used in FBM generation.
 * @param  peak_kw        Base spatial frequency of the main ridge/peak
 *                        structure.
 * @param  rugosity       Controls the roughness of base noise displacements.
 * @param  angle          Orientation angle (in degrees) of the main ridge
 *                        direction.
 * @param  k_smoothing    Smoothing coefficient applied to the Voronoi FBM
 *                        ridges.
 * @param  gamma          Gamma correction applied to the ridge intensity.
 * @param  add_deposition If true, applies an additional smoothing pass to
 *                        simulate sediment deposition.
 * @param  ridge_amp      Amplitude scaling factor for ridge prominence.
 * @param  base_noise_amp Amplitude scaling factor for base displacement noise.
 * @param  center         Normalized coordinates of the mountain’s center
 *                        (default domain: [0, 1]²).
 * @param  p_noise_x      Optional pointer to horizontal displacement noise
 *                        (nullptr for none).
 * @param  p_noise_y      Optional pointer to vertical displacement noise
 *                        (nullptr for none).
 * @param  bbox           Bounding box of the generated region (xmin, xmax,
 *                        ymin, ymax).
 *
 * @return                A 2D Array containing the generated mountain stump
 *                        heightmap.
 *
 * **Example**
 * @include ex_mountain_stump.cpp
 *
 * **Result**
 * @image html ex_mountain_stump.png
 * */
Array mountain_stump(Vec2<int>    shape,
                     uint         seed,
                     float        scale = 1.f,
                     int          octaves = 8,
                     float        peak_kw = 6.f,
                     float        rugosity = 0.f,
                     float        angle = 45.f,
                     float        k_smoothing = 0.f,
                     float        gamma = 0.25f,
                     bool         add_deposition = true,
                     float        ridge_amp = 0.75f,
                     float        base_noise_amp = 0.1f,
                     Vec2<float>  center = {0.5f, 0.5f},
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a synthetic "Tibesti" mountain heightmap.
 *
 * This function procedurally creates a 2D elevation field inspired by the
 * Tibesti mountains, a desert volcanic range. The terrain is built by combining
 * Gabor wavelets, fractal simplex noise, and a Gaussian envelope, producing
 * sharp volcanic peaks modulated by erosion-like patterns.
 *
 * @param  shape              Output array shape (resolution in x and y).
 * @param  seed               Random seed used for noise and wavelet generation.
 * @param  scale              Global scaling factor controlling the overall
 *                            mountain size.
 * @param  octaves            Number of octaves used in the fractal noise and
 *                            Gabor FBM.
 * @param  peak_kw            Base frequency of the peak features (scaled
 *                            internally by @p scale).
 * @param  rugosity           Controls roughness of the fractal noise (higher =
 *                            more irregular).
 * @param  angle              Orientation angle (degrees) for Gabor waves.
 * @param  angle_spread_ratio Spread ratio of the Gabor wave orientation
 *                            (controls anisotropy).
 * @param  gamma              Gamma correction factor applied to auxiliary noise
 *                            fields.
 * @param  add_deposition     If true, applies a smoothing fill step simulating
 *                            sediment deposition.
 * @param  bulk_amp           Amplitude of bulk uplift applied to the peaks
 *                            (affects normalization with noise weighting).
 * @param  base_noise_amp     Amplitude of the base displacement noise.
 * @param  center             Center of the mountain in normalized coordinates.
 * @param  p_noise_x          Optional pointer to external displacement noise
 *                            field (X-axis).
 * @param  p_noise_y          Optional pointer to external displacement noise
 *                            field (Y-axis).
 * @param  bbox               Bounding box of the generation domain in
 *                            normalized coordinates.
 *
 * @return                    Array containing the generated Tibesti mountain
 *                            heightmap.
 *
 * **Example**
 * @include ex_mountain_tibesti.cpp
 *
 * **Result**
 * @image html ex_mountain_tibesti.png
 */
Array mountain_tibesti(Vec2<int>    shape,
                       uint         seed,
                       float        scale = 1.f,
                       int          octaves = 8,
                       float        peak_kw = 20.f,
                       float        rugosity = 0.f,
                       float        angle = 30.f,
                       float        angle_spread_ratio = 0.25f,
                       float        gamma = 1.f,
                       bool         add_deposition = true,
                       float        bulk_amp = 1.f,
                       float        base_noise_amp = 0.1f,
                       Vec2<float>  center = {0.5f, 0.5f},
                       const Array *p_noise_x = nullptr,
                       const Array *p_noise_y = nullptr,
                       Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief GPU-accelerated multi-step height generation with procedural noise.
 *
 * Builds the same multi-step profile as the CPU version but injects a
 * Voronoi-FBM noise field to modulate the step positions. Noise can be inflated
 * or deflated and is projected onto the step axis before being passed to the
 * CPU multisteps() implementation.
 *
 * @param  shape              Output array resolution.
 * @param  angle              Rotation angle in degrees.
 * @param  seed               Noise seed.
 * @param  kw                 Base wave numbers for noise.
 * @param  noise_amp          Amplitude of injected noise.
 * @param  noise_rugosity     Roughness of the FBM noise.
 * @param  noise_inflate      Whether to invert noise direction.
 * @param  r                  Geometric ratio between steps.
 * @param  nsteps             Number of steps.
 * @param  elevation_exponent Exponent shaping step heights.
 * @param  shape_gain         Gain used to reshape intra-step transitions.
 * @param  scale              Scale of the step axis.
 * @param  outer_slope        Slope outside the [0,1] axis interval.
 * @param  p_ctrl_param       Optional control parameter array.
 * @param  center             Center of the step axis.
 * @param  bbox               Bounding box of the domain.
 * @return                    Generated Array.
 *
 * **Example**
 * @include ex_multisteps.cpp
 *
 * **Result**
 * @image html ex_multisteps.png
 */
Array multisteps(Vec2<int>          shape,
                 float              angle,
                 uint               seed,
                 Vec2<float>        kw = {2.f, 2.f},
                 float              noise_amp = 0.1f,
                 float              noise_rugosity = 0.f,
                 bool               noise_inflate = true,
                 float              r = 1.2f,
                 int                nsteps = 8,
                 float              elevation_exponent = 0.7f,
                 float              shape_gain = 4.f,
                 float              scale = 0.5f,
                 float              outer_slope = 0.1f,
                 const Array       *p_ctrl_param = nullptr,
                 const Vec2<float> &center = {0.5f, 0.5f},
                 const Vec4<float> &bbox = {0.f, 1.f, 0.f, 1.f});

/*! @brief See hmap::noise */
Array noise(NoiseType    noise_type,
            Vec2<int>    shape,
            Vec2<float>  kw,
            uint         seed,
            const Array *p_noise_x = nullptr,
            const Array *p_noise_y = nullptr,
            const Array *p_stretching = nullptr,
            Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/*! @brief See hmap::noise_fbm */
Array noise_fbm(NoiseType    noise_type,
                Vec2<int>    shape,
                Vec2<float>  kw,
                uint         seed,
                int          octaves = 8,
                float        weight = 0.7f,
                float        persistence = 0.5f,
                float        lacunarity = 2.f,
                const Array *p_ctrl_param = nullptr,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                const Array *p_stretching = nullptr,
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a scalar field representing the signed distance to randomly
 * generated polygons.
 *
 * This function procedurally generates a field where each pixel stores the
 * signed distance to the nearest polygon, using a smooth signed distance
 * function (SDF) with optional clamping. The polygons are generated randomly
 * within the specified bounding box, with configurable vertex counts, sizes,
 * jitter, and density. Optionally, displacement noise fields can be applied to
 * polygon positions.
 *
 * @param  shape          Resolution of the output field (width, height).
 * @param  kw             Scaling factor for field coordinates (world units per
 *                        pixel).
 * @param  seed           Random seed for polygon generation.
 * @param  rmin           Minimum polygon radius (relative to bbox size).
 * @param  rmax           Maximum polygon radius (relative to bbox size).
 * @param  clamping_dist  Distance threshold for clamping the SDF value (used to
 *                        soften edges).
 * @param  clamping_k     Smoothness parameter for clamping.
 * @param  n_vertices_min Minimum number of vertices per polygon.
 * @param  n_vertices_max Maximum number of vertices per polygon.
 * @param  density        Fraction of pixels covered by polygons (approximate).
 * @param  jitter         Random displacement factor applied to polygon
 *                        vertices.
 * @param  shift          Random position shift applied to polygon center.
 * @param  p_noise_x      Optional displacement noise field in the X direction
 *                        (nullptr to disable).
 * @param  p_noise_y      Optional displacement noise field in the Y direction
 *                        (nullptr to disable).
 * @param  bbox           Bounding box in normalized coordinates {xmin, xmax,
 *                        ymin, ymax}.
 * @return                Array         2D array containing the signed distance
 *                        field.
 *
 * @note Polygons are randomly generated per call and are not guaranteed to be
 * convex.
 * @note The sign of the SDF is negative inside polygons and positive outside.
 *
 * **Example**
 * @include ex_polygon_field_fbm.cpp
 *
 * **Result**
 * @image html ex_polygon_field_fbm.png
 */
Array polygon_field(Vec2<int>         shape,
                    Vec2<float>       kw,
                    uint              seed,
                    float             rmin = 0.05f,
                    float             rmax = 0.8f,
                    float             clamping_dist = 0.1f,
                    float             clamping_k = 0.1f,
                    int               n_vertices_min = 3,
                    int               n_vertices_max = 16,
                    float             density = 0.5f,
                    hmap::Vec2<float> jitter = {0.5f, 0.5f},
                    float             shift = 0.1f,
                    const Array      *p_noise_x = nullptr,
                    const Array      *p_noise_y = nullptr,
                    const Array      *p_noise_distance = nullptr,
                    const Array      *p_density_multiplier = nullptr,
                    const Array      *p_size_multiplier = nullptr,
                    Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a scalar field representing the signed distance to randomly
 * generated polygons combined with fractal Brownian motion (fBm) noise
 * modulation.
 *
 * Similar to polygon_field(), but the generated SDF is modulated by an fBm
 * noise function to create more natural, irregular shapes. The fBm parameters
 * allow control over the noise persistence, lacunarity, and number of octaves.
 *
 * @param  shape          Resolution of the output field (width, height).
 * @param  kw             Scaling factor for field coordinates (world units per
 *                        pixel).
 * @param  seed           Random seed for polygon generation and fBm noise.
 * @param  rmin           Minimum polygon radius (relative to bbox size).
 * @param  rmax           Maximum polygon radius (relative to bbox size).
 * @param  clamping_dist  Distance threshold for clamping the SDF value (used to
 *                        soften edges).
 * @param  clamping_k     Smoothness parameter for clamping.
 * @param  n_vertices_min Minimum number of vertices per polygon.
 * @param  n_vertices_max Maximum number of vertices per polygon.
 * @param  density        Fraction of pixels covered by polygons (approximate).
 * @param  jitter         Random displacement factor applied to polygon
 *                        vertices.
 * @param  shift          Random position shift applied to polygon center.
 * @param  octaves        Number of fBm octaves.
 * @param  persistence    Amplitude decay per octave in fBm.
 * @param  lacunarity     Frequency multiplier per octave in fBm.
 * @param  p_noise_x      Optional displacement noise field in the X direction
 *                        (nullptr to disable).
 * @param  p_noise_y      Optional displacement noise field in the Y direction
 *                        (nullptr to disable).
 * @param  bbox           Bounding box in normalized coordinates {xmin, xmax,
 *                        ymin, ymax}.
 * @return                Array         2D array containing the signed distance
 *                        field modulated by fBm.
 *
 * @note The sign of the SDF is negative inside polygons and positive outside.
 *
 * **Example**
 * @include ex_polygon_field_fbm.cpp
 *
 * **Result**
 * @image html ex_polygon_field_fbm.png
 */
Array polygon_field_fbm(Vec2<int>         shape,
                        Vec2<float>       kw,
                        uint              seed,
                        float             rmin = 0.05f,
                        float             rmax = 0.8f,
                        float             clamping_dist = 0.1f,
                        float             clamping_k = 0.1f,
                        int               n_vertices_min = 3,
                        int               n_vertices_max = 16,
                        float             density = 0.1f,
                        hmap::Vec2<float> jitter = {0.5f, 0.5f},
                        float             shift = 0.1f,
                        int               octaves = 8,
                        float             persistence = 0.5f,
                        float             lacunarity = 2.f,
                        const Array      *p_noise_x = nullptr,
                        const Array      *p_noise_y = nullptr,
                        const Array      *p_noise_distance = nullptr,
                        const Array      *p_density_multiplier = nullptr,
                        const Array      *p_size_multiplier = nullptr,
                        Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a synthetic "shattered peak" terrain heightmap.
 *
 * This function procedurally creates a 2D elevation field resembling a sharp,
 * fractured mountain peak. It combines a Gaussian pulse envelope, fractal noise
 * displacements, and Voronoi-based primitives with edge-distance shaping. The
 * result is a rugged peak structure with broken ridges and steep slopes.
 *
 * @param  shape          Output array shape (resolution in x and y).
 * @param  seed           Random seed used for noise and Voronoi generation.
 * @param  scale          Global scaling factor controlling the overall peak
 *                        size.
 * @param  peak_kw        Base frequency of the peak features (scaled internally
 *                        by @p scale).
 * @param  rugosity       Controls roughness of the fractal noise (higher = more
 *                        irregular).
 * @param  angle          Orientation angle (degrees) for directional
 *                        displacements.
 * @param  gamma          Gamma correction factor applied to the final
 *                        heightmap.
 * @param  add_deposition If true, applies a smoothing fill step simulating
 *                        sediment deposition.
 * @param  bulk_amp       Amplitude of bulk uplift applied to the peak
 *                        (internally overridden to 0.5f for normalization).
 * @param  base_noise_amp Amplitude of the base displacement noise.
 * @param  k_smoothing    Voronoi smoothing parameter (controls ridge
 *                        sharpness).
 * @param  center         Center of the peak in normalized coordinates.
 * @param  p_noise_x      Optional pointer to external displacement noise field
 *                        (X-axis).
 * @param  p_noise_y      Optional pointer to external displacement noise field
 *                        (Y-axis).
 * @param  bbox           Bounding box of the generation domain in normalized
 *                        coordinates.
 *
 * @return                Array containing the generated shattered peak
 *                        heightmap.
 *
 * **Example**
 * @include ex_shattered_peak.cpp
 *
 * **Result**
 * @image html ex_shattered_peak.png
 */
Array shattered_peak(Vec2<int>    shape,
                     uint         seed,
                     float        scale = 1.f,
                     int          octaves = 8,
                     float        peak_kw = 4.f,
                     float        rugosity = 0.f,
                     float        angle = 30.f,
                     float        gamma = 1.f,
                     bool         add_deposition = true,
                     float        bulk_amp = 0.3f,
                     float        base_noise_amp = 0.1f,
                     float        k_smoothing = 0.f,
                     Vec2<float>  center = {0.5f, 0.5f},
                     const Array *p_noise_x = nullptr,
                     const Array *p_noise_y = nullptr,
                     Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a Voronoi-based pattern where cells are defined by proximity
 * to random lines.
 *
 * This function generates an OpenCL-accelerated Voronoi-like pattern based on
 * the distance from each pixel to a set of randomly oriented lines. Each line
 * is defined by a random point and a direction sampled from a uniform
 * distribution around a given angle.
 *
 * @param  shape       The resolution of the resulting 2D array (width, height).
 * @param  density     Number of base points per unit area used to define lines.
 * @param  seed        Seed for the random number generator used to generate
 *                     base points and directions.
 * @param  k_smoothing Kernel smoothing factor; controls how sharp or soft the
 *                     distance fields are.
 * @param  exp_sigma   Exponential smoothing parameter applied to the computed
 *                     distance field.
 * @param  alpha       Base angle (in radians) used to orient the generated
 *                     lines.
 * @param  alpha_span  Maximum angular deviation from `alpha`; controls line
 *                     orientation variability.
 * @param  return_type Type of Voronoi output to return (e.g., F1, F2, edge
 *                     distance, smoothed field, etc.).
 * @param  p_noise_x   Optional pointer to an input noise field applied to the X
 *                     coordinates (can be nullptr).
 * @param  p_noise_y   Optional pointer to an input noise field applied to the Y
 *                     coordinates (can be nullptr).
 * @param  bbox        Bounding box in normalized coordinates (min_x, max_x,
 *                     min_y, max_y) of the final array.
 * @param  bbox_points Bounding box within which random base points are sampled.
 *
 * @return             A 2D array (of type Array) containing the computed
 *                     distance field based on line proximity.
 *
 * @note Each line is defined from a point (x, y) to a direction offset using
 * angle `theta = alpha + rand * alpha_span`.
 * @note The OpenCL kernel "vorolines" must be defined and compiled beforehand.
 *
 * **Example**
 * @include ex_vorolines.cpp
 *
 * **Result**
 * @image html ex_vorolines.png
 * @image html ex_vorolines_fbm.png
 */
Array vorolines(Vec2<int>         shape,
                float             density,
                uint              seed,
                float             k_smoothing = 0.f,
                float             exp_sigma = 0.f,
                float             alpha = 0.f,
                float             alpha_span = M_PI,
                VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
                const Array      *p_noise_x = nullptr,
                const Array      *p_noise_y = nullptr,
                Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f},
                Vec4<float>       bbox_points = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a Voronoi-based pattern using distances to lines defined by
 * random points and angles, with additional fractal Brownian motion (fBm) noise
 * modulation.
 *
 * This function extends the standard `vorolines` generation by introducing
 * fBm-based warping of the coordinate space, resulting in more organic and
 * fractal-like structures. It creates a Voronoi distance field based on
 * proximity to oriented line segments and distorts the result using
 * multi-octave procedural noise.
 *
 * @param  shape       Output resolution of the 2D array (width, height).
 * @param  density     Number of base points per unit area used to define lines.
 * @param  seed        Seed value for the random number generator.
 * @param  k_smoothing Kernel smoothing coefficient to soften distance values
 *                     (e.g., for blending).
 * @param  exp_sigma   Sigma value for optional exponential smoothing on the
 *                     final field.
 * @param  alpha       Base orientation angle (in radians) of lines generated
 *                     from random points.
 * @param  alpha_span  Maximum angle deviation from `alpha`, determining
 *                     directional randomness of lines.
 * @param  return_type Type of output to return (e.g., F1, F2, distance to edge,
 *                     smoothed version).
 * @param  octaves     Number of noise octaves used in the fBm modulation.
 * @param  weight      Weight of each octave's contribution to the total noise.
 * @param  persistence Amplitude decay factor for each successive octave
 *                     (commonly 0.5–0.8).
 * @param  lacunarity  Frequency multiplier for each successive octave (commonly
 *                     2.0).
 * @param  p_noise_x   Optional pointer to an external noise field applied to X
 *                     coordinates (can be nullptr).
 * @param  p_noise_y   Optional pointer to an external noise field applied to Y
 *                     coordinates (can be nullptr).
 * @param  bbox        Bounding box for the final image domain (min_x, max_x,
 *                     min_y, max_y).
 * @param  bbox_points Bounding box from which the initial set of points are
 *                     sampled.
 *
 * @return             A 2D `Array` representing the Voronoi-fBm field,
 *                     distorted by noise and influenced by distance to random
 *                     lines.
 *
 * @note This version uses internally computed fBm noise unless external fields
 * (`p_noise_x`, `p_noise_y`) are provided.
 * @note This function requires an OpenCL kernel named "vorolines_fbm" to be
 * compiled and accessible.
 *
 * **Example**
 * @include ex_vorolines.cpp
 *
 * **Result**
 * @image html ex_vorolines.png
 * @image html ex_vorolines_fbm.png
 */
Array vorolines_fbm(
    Vec2<int>         shape,
    float             density,
    uint              seed,
    float             k_smoothing = 0.f,
    float             exp_sigma = 0.f,
    float             alpha = 0.f,
    float             alpha_span = M_PI,
    VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
    int               octaves = 8,
    float             weight = 0.7f,
    float             persistence = 0.5f,
    float             lacunarity = 2.f,
    const Array      *p_noise_x = nullptr,
    const Array      *p_noise_y = nullptr,
    Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f},
    Vec4<float>       bbox_points = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a Voronoi diagram in a 2D array with configurable
 * properties.
 *
 * @param  shape        The dimensions of the output array as a 2D vector of
 *                      integers.
 * @param  kw           The frequency scale factors for the Voronoi cells, given
 *                      as a 2D vector of floats.
 * @param  seed         A seed value for random number generation, ensuring
 *                      reproducibility.
 * @param  jitter       (Optional) The amount of random variation in the
 *                      positions of Voronoi cell sites, given as a 2D vector of
 *                      floats. Defaults to {0.5f, 0.5f}.
 * @param  return_type  (Optional) The type of value to compute for the Voronoi
 *                      diagram. Defaults to `VoronoiReturnType::F1_SQUARED`.
 * @param  p_ctrl_param (Optional) A pointer to an `Array` used to control the
 *                      Voronoi computation. Used here as a multiplier for the
 *                      jitter. If nullptr, no control is applied.
 * @param  p_noise_x    (Optional) A pointer to an `Array` providing additional
 *                      noise in the x-direction for cell positions. If nullptr,
 *                      no x-noise is applied.
 * @param  p_noise_y    (Optional) A pointer to an `Array` providing additional
 *                      noise in the y-direction for cell positions. If nullptr,
 *                      no y-noise is applied.
 * @param  bbox         (Optional) The bounding box for the Voronoi computation,
 *                      given as a 4D vector of floats representing {min_x,
 *                      max_x, min_y, max_y}. Defaults to
 * {0.f, 1.f, 0.f, 1.f}.
 *
 * @return              A 2D array representing the generated Voronoi diagram.
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_voronoi.cpp
 *
 * **Result**
 * @image html ex_voronoi.png
 */
Array voronoi(Vec2<int>         shape,
              Vec2<float>       kw,
              uint              seed,
              Vec2<float>       jitter = {0.5f, 0.5f},
              float             k_smoothing = 0.f,
              float             exp_sigma = 0.f,
              VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
              const Array      *p_ctrl_param = nullptr,
              const Array      *p_noise_x = nullptr,
              const Array      *p_noise_y = nullptr,
              Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a Voronoi diagram in a 2D array with configurable
 * properties.
 *
 * @param  shape        The dimensions of the output array as a 2D vector of
 *                      integers.
 * @param  kw           The frequency scale factors for the base Voronoi cells,
 *                      given as a 2D vector of floats.
 * @param  seed         A seed value for random number generation, ensuring
 *                      reproducibility.
 * @param  jitter       (Optional) The amount of random variation in the
 *                      positions of Voronoi cell sites, given as a 2D vector of
 *                      floats. Defaults to {0.5f, 0.5f}.
 * @param  return_type  (Optional) The type of value to compute for the Voronoi
 *                      diagram. Defaults to `VoronoiReturnType::F1_SQUARED`.
 * @param  octaves      (Optional) The number of layers (octaves) in the fractal
 *                      Brownian motion. Defaults to 8.
 * @param  weight       (Optional) The initial weight of the base layer in the
 *                      FBM computation. Defaults to 0.7f.
 * @param  persistence  (Optional) The persistence factor that controls the
 *                      amplitude reduction between octaves. Defaults to 0.5f.
 * @param  lacunarity   (Optional) The lacunarity factor that controls the
 *                      frequency increase between octaves. Defaults to 2.f.
 * @param  p_ctrl_param (Optional) A pointer to an `Array` used to control the
 *                      Voronoi computation. If nullptr, no control is applied.
 * @param  p_noise_x    (Optional) A pointer to an `Array` providing additional
 *                      noise in the x-direction for cell positions. If nullptr,
 *                      no x-noise is applied.
 * @param  p_noise_y    (Optional) A pointer to an `Array` providing additional
 *                      noise in the y-direction for cell positions. If nullptr,
 *                      no y-noise is applied.
 * @param  bbox         (Optional) The bounding box for the Voronoi computation,
 *                      given as a 4D vector of floats representing {min_x,
 *                      max_x, min_y, max_y}. Defaults to
 * {0.f, 1.f, 0.f, 1.f}.
 *
 * @return              A 2D array representing the generated Voronoi diagram.
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_voronoi.cpp
 *
 * **Result**
 * @image html ex_voronoi.png
 */
Array voronoi_fbm(Vec2<int>         shape,
                  Vec2<float>       kw,
                  uint              seed,
                  Vec2<float>       jitter = {0.5f, 0.5f},
                  float             k_smoothing = 0.f,
                  float             exp_sigma = 0.f,
                  VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
                  int               octaves = 8,
                  float             weight = 0.7f,
                  float             persistence = 0.5f,
                  float             lacunarity = 2.f,
                  const Array      *p_ctrl_param = nullptr,
                  const Array      *p_noise_x = nullptr,
                  const Array      *p_noise_y = nullptr,
                  Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Computes the Voronoi edge distance.
 *
 * @param shape                The shape of the grid as a 2D vector (width,
 *                             height).
 * @param kw                   The weights for the Voronoi kernel as a 2D
 *                             vector.
 * @param seed                 The random seed used for generating Voronoi
 *                             points.
 * @param jitter               Optional parameter for controlling jitter in
 *                             Voronoi point placement (default is {0.5f,
 *                             0.5f}).
 * @param p_ctrl_param         Optional pointer to an Array specifying control
 *                             parameters for Voronoi grid jitter (default is
 *                             nullptr).
 * @param p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param bbox                 The bounding box for the Voronoi diagram as
 *                            {x_min, x_max, y_min, y_max} (default is {0.f,
 * 1.f, 0.f, 1.f}).
 *
 * @note Taken from https://www.shadertoy.com/view/llG3zy
 *
 * @note Only available if OpenCL is enabled.
 *
 * @note The resulting Array has the same dimensions as the input shape.
 */
Array voronoi_edge_distance(Vec2<int>    shape,
                            Vec2<float>  kw,
                            uint         seed,
                            Vec2<float>  jitter = {0.5f, 0.5f},
                            const Array *p_ctrl_param = nullptr,
                            const Array *p_noise_x = nullptr,
                            const Array *p_noise_y = nullptr,
                            Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a 2D Voronoi noise array.
 *
 * This function computes a Voronoi noise pattern based on the input parameters
 * and returns it as a 2D array. The noise is calculated in the OpenCL kernel
 * `noise_voronoise`, which uses a combination of hashing and smoothstep
 * functions to generate a weighted Voronoi noise field.
 *
 * @note Taken from https://www.shadertoy.com/view/Xd23Dh
 *
 * @note Only available if OpenCL is enabled.
 *
 * @param  shape                The dimensions of the 2D output array as a
 *                              vector (width and height).
 * @param  kw                   Wave numbers for scaling the noise pattern,
 *                              represented as a 2D vector.
 * @param  u_param              A control parameter for the noise, adjusting the
 *                              contribution of random offsets.
 * @param  v_param              A control parameter for the noise, affecting the
 *                              smoothness of the pattern.
 * @param  p_ctrl_param         Optional pointer to an Array specifying control
 *                              parameters for Voronoi grid jitter (default is
 *                              nullptr).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  seed                 A seed value for random number generation,
 *                              ensuring reproducibility.
 *
 * @return                      An `Array` object containing the generated 2D
 *                              Voronoi noise values.
 *
 * **Example**
 * @include ex_voronoise.cpp
 *
 * **Result**
 * @image html ex_voronoise.png
 */
Array voronoise(Vec2<int>    shape,
                Vec2<float>  kw,
                float        u_param,
                float        v_param,
                uint         seed,
                const Array *p_noise_x = nullptr,
                const Array *p_noise_y = nullptr,
                Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Return an array filled with coherence Voronoise.
 *
 * @param  shape                Array shape.
 * @param  kw                   Noise wavenumbers {kx, ky} for each directions.
 * @param  seed                 Random seed number.
 * @param  octaves              Number of octaves.
 * @param  weigth               Octave weighting.
 * @param  persistence          Octave persistence.
 * @param  lacunarity           Defines the wavenumber ratio between each
 *                              octaves.
 * @param  p_ctrl_param         Reference to the control parameter array (acts
 *                              as a multiplier for the weight parameter).
 * @param  p_noise_x, p_noise_y Reference to the input noise arrays.
 * @param  bbox                 Domain bounding box.
 * @return                      Array Fractal noise.
 *
 * @note Taken from https://www.shadertoy.com/view/clGyWm
 *
 * @note Only available if OpenCL is enabled.
 *
 * **Example**
 * @include ex_voronoise.cpp
 *
 * **Result**
 * @image html ex_voronoise.png
 */
Array voronoise_fbm(Vec2<int>    shape,
                    Vec2<float>  kw,
                    float        u_param,
                    float        v_param,
                    uint         seed,
                    int          octaves = 8,
                    float        weight = 0.7f,
                    float        persistence = 0.5f,
                    float        lacunarity = 2.f,
                    const Array *p_ctrl_param = nullptr,
                    const Array *p_noise_x = nullptr,
                    const Array *p_noise_y = nullptr,
                    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates a 2D Voronoi-based scalar field using OpenCL.
 *
 * This function computes a Voronoi diagram or derived metric (such as F1, F2,
 * or edge distances) on a grid of given shape. A set of random points is
 * generated within an extended bounding box, based on the desired density and
 * variability, to reduce edge artifacts. Optionally, per-pixel displacement can
 * be applied through noise fields.
 *
 * The result is stored in an `Array` object representing a 2D scalar field.
 *
 * @param  shape       Dimensions of the output array (width x height).
 * @param  density     Number of random points per unit area for Voronoi
 *                     diagram.
 * @param  variability Amount of randomness added to the point generation
 *                     bounding box.
 * @param  seed        Seed for random number generation used in point sampling.
 * @param  k_smoothing Smoothing factor used in soft minimum/maximum Voronoi
 *                     distance computations.
 * @param  exp_sigma   Standard deviation used in exponential falloff for edge
 *                     distance computation.
 * @param  return_type Type of Voronoi computation to perform (e.g., F1, F2,
 *                     F2-F1, edge distance).
 * @param  p_noise_x   Optional pointer to a noise field applied to X
 *                     coordinates of grid points.
 * @param  p_noise_y   Optional pointer to a noise field applied to Y
 *                     coordinates of grid points.
 * @param  bbox        Bounding box of the domain in which the field is
 *                     computed: {xmin, xmax, ymin, ymax}.
 * @param  bbox_points Bounding box for point generation, usually larger than
 * `bbox` to avoid edge effects.
 *
 * @return             An `Array` object containing the computed scalar field.
 *
 * @note
 * - The kernel `"vororand"` must be compiled and available in the OpenCL
 * context.
 * - If `p_noise_x` or `p_noise_y` are provided, they must match the shape of
 * the output array.
 * - The generated point cloud will be larger than `bbox` to reduce border
 * artifacts.
 *
 * **Example**
 * @include ex_vororand.cpp
 *
 * **Result**
 * @image html ex_vororand.png
 */
Array vororand(Vec2<int>         shape,
               float             density,
               float             variability,
               uint              seed,
               float             k_smoothing = 0.f,
               float             exp_sigma = 0.f,
               VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
               const Array      *p_noise_x = nullptr,
               const Array      *p_noise_y = nullptr,
               Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f},
               Vec4<float>       bbox_points = {0.f, 1.f, 0.f, 1.f});

Array vororand(Vec2<int>                 shape,
               const std::vector<float> &xp,
               const std::vector<float> &yp,
               float                     k_smoothing = 0.f,
               float                     exp_sigma = 0.f,
               VoronoiReturnType return_type = VoronoiReturnType::F1_SQUARED,
               const Array      *p_noise_x = nullptr,
               const Array      *p_noise_y = nullptr,
               Vec4<float>       bbox = {0.f, 1.f, 0.f, 1.f});

/**
 * @brief Generates 2D wavelet noise using an OpenCL kernel.
 *
 * This function creates a 2D noise field of the given @p shape using
 * multi-octave wavelet turbulence. The noise is computed on the GPU via the
 * "wavelet_noise" OpenCL kernel. Various parameters control the frequency
 * evolution, amplitude decay, vorticity injection, and optional modulation
 * using additional input arrays.
 *
 * @param  shape         Resolution of the output noise array (width, height).
 * @param  kw            Base wave number (frequency) in each axis. Controls the
 *                       initial spatial frequency of the wavelet field.
 * @param  seed          Integer seed used by the random hashing functions
 *                       inside the kernel.
 * @param  kw_multiplier Multiplier applied to the wave number per octave
 *                       (similar to gain). Higher values increase frequency
 *                       growth across octaves.
 * @param  vorticity     Amount of rotational distortion injected into the
 *                       noise. When > 0, the kernel applies additional angular
 *                       perturbations to produce turbulent flow-like patterns.
 * @param  density       Global scaling factor controlling the overall contrast
 *                       or amplitude of the generated noise.
 * @param  octaves       Number of wavelet noise layers combined. Higher octaves
 *                       yield more detail but increase computational cost.
 * @param  weight        Weight applied to the base octave, used as a global
 *                       intensity multiplier before persistence attenuation is
 *                       applied.
 * @param  persistence   Amplitude decay factor per octave. Values in (0,1)
 *                       produce the classic fractal noise falloff; higher
 *                       values retain more high-frequency detail.
 * @param  lacunarity    Frequency multiplier per octave. Controls how rapidly
 *                       frequency increases at each octave, with typical values
 *                       in the range [1.5, 3.0].
 * @param  p_ctrl_param  Optional pointer to a control-parameter array. When
 *                       provided, values inside this array modulate the
 *                       generated noise spatially. Pass nullptr to disable this
 *                       feature.
 * @param  p_noise_x     Optional pointer to an auxiliary noise array used to
 *                       perturb sampling positions horizontally. Pass nullptr
 *                       to disable.
 * @param  p_noise_y     Optional pointer to an auxiliary noise array used to
 *                       perturb sampling positions vertically. Pass nullptr to
 *                       disable.
 * @param  bbox          Bounding-box mapping (xmin, ymin, xmax, ymax). Converts
 *                       pixel-space indices into world-space coordinates for
 *                       spatially consistent noise.
 *
 * @return               Array A 2D floating-point array containing the
 *                       generated wavelet noise.
 *
 * **Example**
 * @include ex_wavelet_noise.cpp
 *
 * **Result**
 * @image html ex_wavelet_noise.png
 */
Array wavelet_noise(Vec2<int>    shape,
                    Vec2<float>  kw,
                    uint         seed,
                    float        kw_multiplier = 2.f,
                    float        vorticity = 0.f,
                    float        density = 1.f,
                    int          octaves = 8,
                    float        weight = 0.7f,
                    float        persistence = 0.5f,
                    float        lacunarity = 2.f,
                    const Array *p_ctrl_param = nullptr,
                    const Array *p_noise_x = nullptr,
                    const Array *p_noise_y = nullptr,
                    Vec4<float>  bbox = {0.f, 1.f, 0.f, 1.f});

} // namespace hmap::gpu
