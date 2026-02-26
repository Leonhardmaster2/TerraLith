# TerraLith / Hesiod — Complete Node Reference

> Auto-generated reference of every node in the engine, its category, functions, data types, and Vulkan GPU support.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Data Types](#data-types)
- [Base Node API (Extension Points)](#base-node-api-extension-points)
- [Vulkan GPU Infrastructure](#vulkan-gpu-infrastructure)
- [Node Inventory](#node-inventory)
  - [Primitive / Coherent Noise](#primitive--coherent-noise)
  - [Primitive / Function](#primitive--function)
  - [Primitive / Geological](#primitive--geological)
  - [Primitive / Random](#primitive--random)
  - [Primitive / Kernel](#primitive--kernel)
  - [Primitive / Authoring](#primitive--authoring)
  - [Operator / Blend](#operator--blend)
  - [Operator / Combiner](#operator--combiner)
  - [Operator / Morphology](#operator--morphology)
  - [Operator / Transform](#operator--transform)
  - [Operator / Tiling](#operator--tiling)
  - [Operator / Resynthesis](#operator--resynthesis)
  - [Filter / Range](#filter--range)
  - [Filter / Recurve](#filter--recurve)
  - [Filter / Recast](#filter--recast)
  - [Filter / Smoothing](#filter--smoothing)
  - [Filter (General)](#filter-general)
  - [Math / Base](#math--base)
  - [Math / Gradient](#math--gradient)
  - [Math / Convolution](#math--convolution)
  - [Math (General)](#math-general)
  - [Erosion / Hydraulic](#erosion--hydraulic)
  - [Erosion / Thermal](#erosion--thermal)
  - [Erosion / Water](#erosion--water)
  - [Erosion / Stratify](#erosion--stratify)
  - [Erosion (General)](#erosion-general)
  - [Features](#features)
  - [Features / Landform](#features--landform)
  - [Features / Clustering](#features--clustering)
  - [Geometry / Cloud](#geometry--cloud)
  - [Geometry / Path](#geometry--path)
  - [Hydrology](#hydrology)
  - [Mask](#mask)
  - [Mask / Selector](#mask--selector)
  - [Mask / ForTexturing](#mask--for-texturing)
  - [Texture](#texture)
  - [Converter](#converter)
  - [IO / Files](#io--files)
  - [Boundaries](#boundaries)
  - [Routing](#routing)
  - [Debug](#debug)
  - [WIP (Work In Progress)](#wip-work-in-progress)

---

## Architecture Overview

The engine is organized in three layers:

| Layer | Purpose | Key Classes |
|-------|---------|-------------|
| **GNode** | Backend graph logic | `gnode::Node`, `gnode::Graph`, `Port`, `Link`, `Data<T>` |
| **GNodeGUI** | Visual node editor (Qt) | `GraphViewer`, `GraphicsNode`, `GraphicsLink`, `NodeProxy` |
| **Hesiod** | Application / terrain nodes | `BaseNode`, `GraphNode`, `NodeFactory`, all 290+ terrain nodes |

Every terrain node inherits from `BaseNode` (which extends `gnode::Node`). Each node is defined by two functions:
- **`setup_<name>_node()`** — Declares ports, attributes, and UI layout
- **`compute_<name>_node()`** — Executes the node's terrain operation (CPU)
- **`compute_<name>_node_vulkan()`** — *(optional)* GPU-accelerated compute via Vulkan

Source location: `Hesiod/src/model/nodes/nodes_function/<name>.cpp`

---

## Data Types

These are the data types that flow through node ports:

| Type | C++ Type | Description |
|------|----------|-------------|
| **Heightmap** | `hmap::Heightmap` | Tiled 2D float grid — the primary terrain data type |
| **HeightmapRGBA** | `hmap::HeightmapRGBA` | 4-channel tiled color map (R, G, B, A heightmaps) |
| **Array** | `hmap::Array` | Single non-tiled 2D float array (kernels, small data) |
| **Cloud** | `hmap::Cloud` | Set of 3D points with values |
| **Path** | `hmap::Path` | Ordered sequence of 3D points with values |

---

## Base Node API (Extension Points)

To extend the engine with a new node, you implement `setup_` and `compute_` functions that operate on `BaseNode`. The key API:

### Port Management
```cpp
node.add_port<T>(gnode::PortType::IN, "name");           // Add input port
node.add_port<T>(gnode::PortType::OUT, "name", CONFIG(node)); // Add output port (with tiling config)
T* node.get_value_ref<T>("port_name");                    // Get data from port
```

### Attribute Management
```cpp
node.add_attr<FloatAttribute>("key", "Label", default, min, max);
node.add_attr<IntAttribute>("key", "Label", default, min, max);
node.add_attr<BoolAttribute>("key", "Label", default);
node.add_attr<EnumAttribute>("key", "Label", enum_map, default);
node.add_attr<SeedAttribute>("key", "Label");
node.add_attr<WaveNbAttribute>("key", "Label");
node.add_attr<RangeAttribute>("key", "Label");
node.add_attr<ColorAttribute>("key", "Label");
node.add_attr<ColorGradientAttribute>("key", "Label", gradient);
node.add_attr<CloudAttribute>("key", "Label", cloud);
node.add_attr<PathAttribute>("key", "Label", path);
node.add_attr<FilenameAttribute>("key", "Label", path, filter);
node.add_attr<MatrixAttribute>("key", "Label", matrix);
// Read attribute values:
auto value = node.get_attr<FloatAttribute>("key");
auto* ref  = node.get_attr_ref<FloatAttribute>("key");
```

### Attribute UI Ordering
```cpp
node.set_attr_ordered_key({
    "_GROUPBOX_BEGIN_Group Name",
    "attr1", "attr2",
    "_GROUPBOX_END_",
    "_SEPARATOR_",
    "attr3"
});
```

### Compute & Configuration
```cpp
node.get_config_ref()->shape;   // Resolution (Vec2<int>)
node.get_config_ref()->tiling;  // Tile count (Vec2<int>)
node.get_config_ref()->overlap; // Tile overlap (float)
CONFIG(node)                    // Macro expanding to shape, tiling, overlap
```

### Vulkan Compute (optional)
```cpp
node.set_compute_vulkan_fct(&compute_<name>_node_vulkan);
node.supports_vulkan_compute();  // Returns true if Vulkan function is set
node.set_vulkan_enabled(bool);   // Toggle GPU on/off per-node
node.is_vulkan_enabled();        // Check if GPU is enabled
node.get_last_backend_used();    // Returns ComputeBackend::CPU, VULKAN, etc.
```

### Post-Processing Helpers
```cpp
post_process_heightmap(node, heightmap);                   // Apply standard post-process chain
post_process_heightmap(node, h, inverse, smooth, ...);     // Explicit parameters
post_apply_enveloppe(node, heightmap, envelope_ptr);       // Apply envelope mask
setup_post_process_heightmap_attributes(node);             // Add standard post-process attributes
pre_process_mask(node, p_mask, h);                         // Pre-process input mask
setup_pre_process_mask_attributes(node);                   // Add mask pre-process attributes
```

### Serialization
```cpp
node.json_to();                  // Serialize node state to JSON
node.json_from(json);            // Deserialize node state from JSON
node.node_parameters_to_json();  // Export parameters only
```

### Special Node Classes
| Class | Inherits | Purpose |
|-------|----------|---------|
| `BaseNode` | `gnode::Node` | Standard terrain node — used by all generic nodes |
| `BroadcastNode` | `BaseNode` | Sends data across graphs via named tags |
| `ReceiveNode` | `BaseNode` | Receives data from a BroadcastNode by tag |

---

## Vulkan GPU Infrastructure

When compiled with `HESIOD_HAS_VULKAN`, GPU-accelerated nodes dispatch compute work through:

| Component | File | Purpose |
|-----------|------|---------|
| `VulkanContext` | `gpu/vulkan/vulkan_context.hpp` | Singleton managing VkInstance, device, queue, command pool, descriptor pool |
| `VulkanBuffer` | `gpu/vulkan/vulkan_buffer.hpp` | RAII wrapper for GPU buffer allocation, upload, and download |
| `VulkanGenericPipeline` | `gpu/vulkan/vulkan_generic_pipeline.hpp` | Singleton that loads `.comp` shaders and dispatches arbitrary compute work |
| `VulkanNoisePipeline` | `gpu/vulkan/vulkan_noise_pipeline.hpp` | Specialized pipeline for noise generation shaders |
| `VulkanErosionPipeline` | `gpu/vulkan/vulkan_erosion_pipeline.hpp` | Specialized pipeline for erosion simulation shaders |
| `VulkanComputeTest` | `gpu/vulkan/vulkan_compute_test.hpp` | Test/validation utilities for GPU compute |

### Compute Shaders

Located in `Hesiod/shaders/`:

| Shader | Used By |
|--------|---------|
| `abs.comp` | Abs |
| `clamp.comp` | Clamp |
| `combiner_add.comp` | CombinerAdd |
| `combiner_blend.comp` | Blend |
| `combiner_divide.comp` | CombinerDivide |
| `combiner_max.comp` | CombinerMax |
| `combiner_min.comp` | CombinerMin |
| `combiner_multiply.comp` | CombinerMultiply |
| `combiner_subtract.comp` | CombinerSubtract |
| `cos.comp` | Cos |
| `fold.comp` | Fold |
| `gabor_wave_fbm.comp` | GaborWaveFbm |
| `gain.comp` | Gain |
| `gamma_correction.comp` | GammaCorrection |
| `gaussian_decay.comp` | GaussianDecay |
| `hydraulic_blur.comp` | HydraulicBlur |
| `hydraulic_erosion.comp` | HydraulicStream |
| `inverse.comp` | Inverse |
| `noise_fbm.comp` | NoiseFbm |
| `normal_map.comp` | HeightmapToNormalMap |
| `remap.comp` | Remap |
| `rescale.comp` | Rescale |
| `saturate.comp` | Saturate |
| `select_slope.comp` | SelectSlope |
| `shift_elevation.comp` | ShiftElevation |
| `smoothstep.comp` | Smoothstep |
| `vector_add.comp` | *(internal utility)* |

---

## Node Inventory

Legend:
- **Vulkan**: Yes = has a GPU compute shader, No = CPU only
- **Ports**: I = Input, O = Output. Type abbreviations: `H` = Heightmap, `RGBA` = HeightmapRGBA, `A` = Array, `C` = Cloud, `P` = Path
- **Setup function**: `setup_<name>_node()` in `nodes_function/<name>.cpp`
- **Compute function**: `compute_<name>_node()` (CPU), `compute_<name>_node_vulkan()` (GPU)

---

### Primitive / Coherent Noise

Procedural noise generators based on coherent noise algorithms.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Dendry** | No | Dendry-based procedural noise |
| **GaborWaveFbm** | **Yes** | Gabor wavelet fractal Brownian motion noise |
| **Gavoronoise** | No | Gavoronoise procedural noise |
| **HemisphereFieldFbm** | No | Hemisphere field with fBm modulation |
| **Noise** | No | Base noise generator (supports multiple noise types via enum: Perlin, Simplex, Value, Worley, etc.) |
| **NoiseFbm** | **Yes** | Fractal Brownian Motion noise |
| **NoiseIq** | No | Inigo Quilez noise variant |
| **NoiseJordan** | No | Jordan noise variant |
| **NoiseParberry** | No | Parberry noise variant |
| **NoisePingpong** | No | Ping-pong folded noise |
| **NoiseRidged** | No | Ridged multifractal noise |
| **NoiseSwiss** | No | Swiss cheese noise variant |
| **PolygonField** | No | Polygon-based field noise |
| **PolygonFieldFbm** | No | Polygon field with fBm modulation |
| **Stamping** | No | Pattern stamping noise |
| **Vorolines** | No | Voronoi-based line patterns |
| **VorolinesFbm** | No | Voronoi lines with fBm modulation |
| **Voronoi** | No | Voronoi cell noise |
| **VoronoiFbm** | No | Voronoi with fBm modulation |
| **Voronoise** | No | Voronoi-noise hybrid |
| **Vororand** | No | Voronoi with random cell values |
| **WaveletNoise** | No | Wavelet-based noise |

---

### Primitive / Function

Mathematical function primitives for generating base shapes.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Bump** | No | Smooth bump/mound shape |
| **BumpLorentzian** | No | Lorentzian-profile bump |
| **Cone** | No | Cone shape |
| **ConeComplex** | No | Multi-parameter cone |
| **ConeSigmoid** | No | Cone with sigmoid falloff |
| **Constant** | No | Flat constant value output |
| **GaussianPulse** | No | Gaussian pulse shape |
| **Paraboloid** | No | Paraboloid surface |
| **Rift** | No | Rift/valley shape |
| **Slope** | No | Linear slope/gradient |
| **Step** | No | Step function |
| **WaveDune** | No | Dune-like wave pattern |
| **WaveSine** | No | Sine wave pattern |
| **WaveSquare** | No | Square wave pattern |
| **WaveTriangular** | No | Triangular wave pattern |

---

### Primitive / Geological

Terrain primitives modeled after real geological formations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Badlands** | No | Badlands terrain generator |
| **BasaltField** | No | Basalt column field terrain |
| **Caldera** | No | Volcanic caldera formation |
| **Crater** | No | Impact crater shape |
| **Island** | No | Island terrain generator |
| **IslandLandMask** | No | Island with land/sea mask |
| **MountainCone** | No | Conical mountain shape |
| **MountainInselberg** | No | Inselberg (isolated rock hill) |
| **MountainRangeRadial** | No | Radial mountain range |
| **MountainStump** | No | Stump mountain shape |
| **MountainTibesti** | No | Tibesti-style mountain |
| **Multisteps** | No | Multi-step terrain formation |
| **ShatteredPeak** | No | Shattered/fractured peak |

---

### Primitive / Random

Random noise and point distributions.

| Node | Vulkan | Description |
|------|--------|-------------|
| **White** | No | White noise (uniform random) |
| **WhiteDensityMap** | No | White noise with density map control |
| **WhiteSparse** | No | Sparse white noise (fewer random points) |

---

### Primitive / Kernel

Convolution kernels for filtering operations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **KernelDiskSmooth** | No | Smooth disk kernel |
| **KernelGabor** | No | Gabor filter kernel |
| **KernelPrim** | No | Primitive kernel shapes |

---

### Primitive / Authoring

Interactive and manual terrain authoring tools.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Brush** | No | Interactive brush painting on heightmap |
| **Ridgelines** | No | Manual ridgeline drawing tool |

---

### Operator / Blend

Blending operations for combining heightmaps.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Blend** | **Yes** | Two-input blend with 13 methods (add, multiply, overlay, min/max smooth, etc.) |
| **Blend3** | No | Three-input blending |
| **BlendPoissonBf** | No | Poisson-based blending with boundary conditions |
| **Mixer** | No | Multi-input mixer with weighted blend |
| **Transfer** | No | Transfer function blend |

---

### Operator / Combiner

Element-wise arithmetic operations on heightmaps.

| Node | Vulkan | Description |
|------|--------|-------------|
| **CombinerAdd** | **Yes** | Element-wise addition (A + B) |
| **CombinerSubtract** | **Yes** | Element-wise subtraction (A - B) |
| **CombinerMultiply** | **Yes** | Element-wise multiplication (A * B) |
| **CombinerDivide** | **Yes** | Element-wise division (A / B) |
| **CombinerMin** | **Yes** | Element-wise minimum |
| **CombinerMax** | **Yes** | Element-wise maximum |

---

### Operator / Morphology

Morphological operations on heightmaps.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Border** | No | Extract border regions |
| **Closing** | No | Morphological closing (dilation then erosion) |
| **Dilation** | No | Morphological dilation |
| **DistanceTransform** | No | Distance transform from binary mask |
| **Erosion** | No | Morphological erosion |
| **MakeBinary** | No | Threshold to binary mask |
| **MorphologicalGradient** | No | Morphological gradient (dilation - erosion) |
| **MorphologicalTopHat** | No | Top-hat transform (original - opening) |
| **Opening** | No | Morphological opening (erosion then dilation) |
| **RelativeDistanceFromSkeleton** | No | Distance field relative to skeleton |
| **Skeleton** | No | Morphological skeleton extraction |

---

### Operator / Transform

Spatial transformation operations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Rotate** | No | Rotate heightmap |
| **Translate** | No | Translate/shift heightmap |
| **Warp** | No | Warp heightmap using displacement maps |
| **Zoom** | No | Zoom/scale heightmap |

---

### Operator / Tiling

Tiling and periodicity operations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **MakePeriodic** | No | Make heightmap tileable |
| **MakePeriodicStitching** | No | Make tileable via stitching method |

---

### Operator / Resynthesis

Texture resynthesis operations on heightmaps.

| Node | Vulkan | Description |
|------|--------|-------------|
| **QuiltingBlend** | No | Quilting-based blending resynthesis |
| **QuiltingExpand** | No | Quilting-based expansion/upscale |
| **QuiltingShuffle** | No | Quilting-based shuffle resynthesis |

---

### Filter / Range

Value range manipulation filters.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Clamp** | **Yes** | Clamp values to a range |
| **ClampOblique** | No | Oblique clamping |
| **Remap** | **Yes** | Remap values to a new range |
| **Rescale** | **Yes** | Rescale values to [0, 1] or custom range |
| **ShiftElevation** | **Yes** | Shift all values by a constant offset |

---

### Filter / Recurve

Curve-based remapping filters.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Gain** | **Yes** | Contrast gain curve |
| **GammaCorrection** | **Yes** | Gamma correction curve |
| **GammaCorrectionLocal** | No | Spatially-varying gamma correction |
| **Plateau** | No | Plateau/shelf curve |
| **Recurve** | No | Custom curve remapping |
| **RecurveKura** | No | Kura-style recurve |
| **RecurveS** | No | S-curve remapping |
| **ReverseAboveThreshold** | No | Reverse values above a threshold |
| **Saturate** | **Yes** | Saturation/compression curve |
| **Terrace** | No | Terrace/staircase quantization |

---

### Filter / Recast

Surface recast and detail manipulation filters.

| Node | Vulkan | Description |
|------|--------|-------------|
| **ExpandShrink** | No | Expand or shrink features |
| **Fold** | **Yes** | Fold/reflect values at threshold |
| **NormalDisplacement** | No | Displace along surface normals |
| **RecastCanyon** | No | Add canyon-like features |
| **RecastCliff** | No | Add cliff features |
| **RecastCliffDirectional** | No | Directional cliff features |
| **RecastCracks** | No | Add surface crack patterns |
| **RecastSag** | No | Add sagging deformation |
| **SteepenConvective** | No | Convective steepening |

---

### Filter / Smoothing

Smoothing and sharpening filters.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Laplace** | No | Laplacian smoothing |
| **Median3x3** | No | 3x3 median filter |
| **ShapeIndex** | No | Shape index computation |
| **SharpenCone** | No | Cone-based sharpening |
| **SmoothCpulse** | No | Cosine-pulse smoothing |
| **SmoothFill** | No | Fill-based smoothing |
| **SmoothFillHoles** | No | Fill holes smoothing |
| **SmoothFillSmearPeaks** | No | Smear peaks smoothing |
| **Smoothstep** | **Yes** | Hermite smoothstep interpolation |

---

### Filter (General)

| Node | Vulkan | Description |
|------|--------|-------------|
| **PostProcess** | No | Configurable post-processing chain (inverse, smooth, saturate, remap) |

---

### Math / Base

Basic mathematical operations on heightmaps.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Abs** | **Yes** | Absolute value with vertical shift |
| **AbsSmooth** | No | Smooth absolute value |
| **Cos** | **Yes** | Cosine function |
| **GaussianDecay** | **Yes** | Gaussian decay function |
| **Inverse** | **Yes** | Multiplicative inverse (1/x) |
| **Lerp** | No | Linear interpolation between two inputs |
| **Reverse** | No | Reverse/negate values |
| **Smoothstep** | **Yes** | *(also listed under Filter/Smoothing)* |

---

### Math / Gradient

Gradient and slope computation.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Gradient** | No | Compute gradient (dx, dy components) |
| **GradientAngle** | No | Gradient angle (aspect) |
| **GradientNorm** | No | Gradient magnitude (slope) |
| **GradientTalus** | No | Talus (maximum slope) |

---

### Math / Convolution

Convolution operations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **ConvolveSVD** | No | SVD-based convolution |

---

### Math (General)

| Node | Vulkan | Description |
|------|--------|-------------|
| **RadialDisplacementToXy** | No | Convert radial displacement to XY components |
| **RotateDisplacement** | No | Rotate displacement vectors |

---

### Erosion / Hydraulic

Water-based hydraulic erosion simulations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **HydraulicBlur** | **Yes** | Hydraulic blur erosion |
| **HydraulicParticle** | No | Particle-based hydraulic erosion |
| **HydraulicStream** | **Yes** | Stream power hydraulic erosion |
| **HydraulicStreamLog** | No | Logarithmic stream power erosion |
| **Rifts** | No | Rift/channel erosion |

---

### Erosion / Thermal

Temperature and gravity-driven erosion simulations.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Thermal** | No | Basic thermal erosion (talus angle) |
| **ThermalAutoBedrock** | No | Thermal erosion with auto-bedrock |
| **ThermalInflate** | No | Thermal erosion with inflation |
| **ThermalRidge** | No | Thermal ridge erosion |
| **ThermalScree** | No | Thermal scree deposition |

---

### Erosion / Water

Coastal and water-level erosion.

| Node | Vulkan | Description |
|------|--------|-------------|
| **CoastalErosionDiffusion** | No | Diffusion-based coastal erosion |
| **CoastalErosionProfile** | No | Profile-based coastal erosion |

---

### Erosion / Stratify

Stratification and layering effects.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Strata** | No | Geological strata layering |

---

### Erosion (General)

| Node | Vulkan | Description |
|------|--------|-------------|
| **DepressionFilling** | No | Fill terrain depressions (sinks) |

---

### Features

Terrain feature detection and analysis.

| Node | Vulkan | Description |
|------|--------|-------------|
| **CurvatureMean** | No | Mean curvature computation |
| **Ruggedness** | No | Terrain ruggedness index |
| **Rugosity** | No | Surface rugosity measurement |
| **StdLocal** | No | Local standard deviation |
| **ZScore** | No | Statistical z-score |

---

### Features / Landform

Landform classification and analysis.

| Node | Vulkan | Description |
|------|--------|-------------|
| **AccumulationCurvature** | No | Accumulation curvature analysis |
| **RelativeElevation** | No | Relative elevation computation |
| **Unsphericity** | No | Unsphericity index |
| **ValleyWidth** | No | Valley width estimation |

---

### Features / Clustering

Spatial clustering algorithms.

| Node | Vulkan | Description |
|------|--------|-------------|
| **KmeansClustering2** | No | K-means clustering (2 clusters) |
| **KmeansClustering3** | No | K-means clustering (3 clusters) |

---

### Geometry / Cloud

Point cloud creation and manipulation.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Cloud** | No | Manual point cloud definition |
| **CloudFromCsv** | No | Import cloud from CSV file |
| **CloudLattice** | No | Regular lattice point cloud |
| **CloudMerge** | No | Merge multiple point clouds |
| **CloudRandom** | No | Uniform random point cloud |
| **CloudRandomDensity** | No | Random cloud with density control |
| **CloudRandomDistance** | No | Random cloud with minimum distance |
| **CloudRandomPowerLaw** | No | Random cloud with power-law distribution |
| **CloudRandomWeibull** | No | Random cloud with Weibull distribution |
| **CloudRemapValues** | No | Remap cloud point values |
| **CloudSDF** | No | Signed distance field from cloud |
| **CloudSetValuesFromBorderDistance** | No | Set values based on border distance |
| **CloudSetValuesFromHeightmap** | No | Sample heightmap at cloud points |
| **CloudSetValuesFromMinDistance** | No | Set values from minimum inter-point distance |
| **CloudShuffle** | No | Shuffle cloud point order |
| **CloudToArrayInterp** | No | Interpolate cloud to array grid |
| **CloudToPath** | No | Convert cloud to path (ordered) |
| **CloudToVectors** | No | Extract cloud as vector arrays |

---

### Geometry / Path

Path creation, editing, and conversion.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Path** | No | Manual path definition |
| **PathBezier** | No | Bezier curve path |
| **PathBezierRound** | No | Rounded Bezier path |
| **PathBspline** | No | B-spline path |
| **PathDecasteljau** | No | De Casteljau subdivision path |
| **PathDecimate** | No | Reduce path point count |
| **PathDig** | No | Dig/carve terrain along path |
| **PathFind** | No | Pathfinding (shortest path on terrain) |
| **PathFractalize** | No | Add fractal detail to path |
| **PathFromCsv** | No | Import path from CSV file |
| **PathMeanderize** | No | Add meander curves to path |
| **PathResample** | No | Resample path at regular intervals |
| **PathSDF** | No | Signed distance field from path |
| **PathShuffle** | No | Shuffle path point order |
| **PathSmooth** | No | Smooth path (reduce sharp turns) |
| **PathToCloud** | No | Convert path to point cloud |
| **PathToHeightmap** | No | Rasterize path onto heightmap |

---

### Hydrology

Water simulation and hydrological analysis.

| Node | Vulkan | Description |
|------|--------|-------------|
| **FlatbedCarve** | No | Carve flat-bed river channels |
| **FloodingFromBoundaries** | No | Flood simulation from boundary edges |
| **FloodingFromPoint** | No | Flood simulation from a point source |
| **FloodingLakeSystem** | No | Lake system flooding simulation |
| **FloodingUniformLevel** | No | Uniform water level flooding |
| **MergeWaterDepths** | No | Merge multiple water depth maps |
| **WaterDepthDryOut** | No | Water depth dry-out simulation |
| **WaterDepthFromMask** | No | Generate water depth from binary mask |
| **WaterMask** | No | Generate water/land binary mask |

---

### Mask

Mask generation and combination.

| Node | Vulkan | Description |
|------|--------|-------------|
| **CombineMask** | No | Combine masks (union, intersection, exclusion) |
| **ScanMask** | No | Scan/sweep mask generation |

---

### Mask / Selector

Automatic mask generation from terrain properties.

| Node | Vulkan | Description |
|------|--------|-------------|
| **SelectAngle** | No | Select by slope angle |
| **SelectBlobLog** | No | Select blobs using Laplacian of Gaussian |
| **SelectCavities** | No | Select concave regions (cavities) |
| **SelectGt** | No | Select values greater than threshold |
| **SelectInterval** | No | Select values within an interval |
| **SelectInwardOutward** | No | Select inward/outward facing slopes |
| **SelectMidrange** | No | Select mid-range elevation values |
| **SelectMultiband3** | No | Three-band selection |
| **SelectPulse** | No | Pulse-shaped selection |
| **SelectRivers** | No | Select river channel locations |
| **SelectSlope** | **Yes** | Select by slope steepness |
| **SelectTransitions** | No | Select transition zones |
| **SelectValley** | No | Select valley regions |

---

### Mask / For Texturing

Specialized masks designed for terrain texturing workflows.

| Node | Vulkan | Description |
|------|--------|-------------|
| **SelectSoilFlow** | No | Soil flow mask for texturing |
| **SelectSoilRocks** | No | Rocky soil mask for texturing |
| **SelectSoilWeathered** | No | Weathered soil mask for texturing |

---

### Texture

RGBA texture generation and manipulation.

| Node | Vulkan | Description |
|------|--------|-------------|
| **ColorizeCmap** | No | Colorize heightmap using a colormap |
| **ColorizeGradient** | No | Colorize using a custom gradient |
| **ColorizeSolid** | No | Solid color output |
| **MixNormalMap** | No | Mix/blend normal maps |
| **MixTexture** | No | Mix/blend RGBA textures |
| **SetAlpha** | No | Set alpha channel of texture |
| **TextureAdvectionParticle** | No | Particle advection texture |
| **TextureQuiltingExpand** | No | Expand texture via quilting |
| **TextureQuiltingShuffle** | No | Shuffle texture via quilting |
| **TextureSplitChannels** | No | Split RGBA into separate heightmaps |
| **TextureToHeightmap** | No | Convert texture to grayscale heightmap |
| **TextureUvChecker** | No | UV checker pattern texture |

---

### Converter

Data type conversion nodes.

| Node | Vulkan | Description |
|------|--------|-------------|
| **HeightmapToKernel** | No | Convert heightmap to convolution kernel (Array) |
| **HeightmapToMask** | No | Convert heightmap to binary mask |
| **HeightmapToNormalMap** | **Yes** | Compute normal map from heightmap |
| **HeightmapToRGBA** | No | Convert heightmap to RGBA texture |
| **NormalMapToHeightmap** | No | Reconstruct heightmap from normal map |

---

### IO / Files

Import and export nodes for file I/O.

| Node | Vulkan | Description |
|------|--------|-------------|
| **ExportAsset** | No | Export as 3D asset (mesh + texture) |
| **ExportCloud** | No | Export point cloud data |
| **ExportCloudToPly** | No | Export point cloud to PLY format |
| **ExportHeightmap** | No | Export heightmap (PNG 8/16-bit, RAW 16-bit, R16, R32) |
| **ExportNormalMap** | No | Export normal map image |
| **ExportPath** | No | Export path data |
| **ExportPointsToPly** | No | Export points to PLY format |
| **ExportTexture** | No | Export RGBA texture image |
| **ImportHeightmap** | No | Import heightmap from image file |
| **ImportTexture** | No | Import RGBA texture from image file |

---

### Boundaries

Edge and boundary treatment nodes.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Bulkify** | No | Bulk up boundaries |
| **Falloff** | No | Apply edge falloff/fade |
| **SetBorders** | No | Set border values explicitly |
| **ZeroedEdges** | No | Force edges to zero |

---

### Routing

Graph data routing and flow control.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Broadcast** | No | Send data to Receive nodes across graphs via named tag. *Uses `BroadcastNode` class.* |
| **Receive** | No | Receive data from a Broadcast node by tag. *Uses `ReceiveNode` class.* |
| **Thru** | No | Pass-through (no-op, for routing convenience) |
| **Toggle** | No | Toggle between two inputs |

---

### Debug

Debugging and inspection nodes.

| Node | Vulkan | Description |
|------|--------|-------------|
| **Debug** | No | Log debug information about input data |
| **Preview** | No | Preview heightmap in the node editor |

---

### WIP (Work In Progress)

These nodes exist in the inventory but are marked as WIP. They may be incomplete or unstable.

| Node | Intended Category | Vulkan | Description |
|------|-------------------|--------|-------------|
| **Detrend** | Filter/Recurve | No | Remove linear trend from heightmap |
| **DiffusionLimitedAggregation** | Primitive/Coherent | No | DLA fractal growth simulation |
| **DirectionalBlur** | Filter/Smoothing | No | Directional blur filter |
| **ExportAsCubemap** | IO/Files | No | Export as cubemap texture |
| **FillTalus** | Operator/Transform | No | Fill to talus angle |
| **FlowStream** | Hydrology | No | Flow stream computation |
| **HydraulicBlur** | Erosion/Hydraulic | **Yes** | Hydraulic blur erosion *(has Vulkan but marked WIP)* |
| **HydraulicMusgrave** | Erosion/Hydraulic | No | Musgrave hydraulic erosion |
| **HydraulicProcedural** | Erosion/Hydraulic | No | Procedural hydraulic erosion |
| **HydraulicSchott** | Erosion/Hydraulic | No | Schott hydraulic erosion |
| **HydraulicStream** | Erosion/Hydraulic | **Yes** | Stream hydraulic erosion *(has Vulkan but marked WIP)* |
| **HydraulicStreamUpscaleAmplification** | Erosion/Hydraulic | No | Upscale amplification for stream erosion |
| **HydraulicVpipes** | Erosion/Hydraulic | No | Virtual pipes hydraulic erosion |
| **Kuwahara** | Filter/Smoothing | No | Kuwahara filter |
| **MeanShift** | — | No | Mean-shift clustering/smoothing |
| **MedianPseudo** | — | No | Pseudo-median filter |
| **ReverseMidpoint** | Primitive/Authoring | No | Reverse midpoint displacement |
| **SedimentDeposition** | Erosion/Deposition | No | Sediment deposition simulation |
| **Stratify** | Erosion/Stratify | No | Stratification effect |
| **StratifyMultiscale** | Erosion/Stratify | No | Multi-scale stratification |
| **StratifyOblique** | Erosion/Stratify | No | Oblique stratification |
| **TextureAdvectionWarp** | Texture | No | Texture advection with warping |
| **ThermalFlatten** | Erosion/Thermal | No | Thermal flattening erosion |
| **ThermalRib** | Erosion/Thermal | No | Thermal rib erosion |
| **ThermalSchott** | Erosion/Thermal | No | Schott thermal erosion |
| **WarpDownslope** | Operator/Transform | No | Downslope warp transformation |
| **Wrinkle** | Filter/Recast | No | Surface wrinkle effect |

---

## Summary Statistics

| Metric | Count |
|--------|-------|
| **Total nodes** | 292 |
| **Nodes with Vulkan GPU support** | 26 |
| **CPU-only nodes** | 266 |
| **WIP nodes** | 27 |
| **Production-ready nodes** | 265 |
| **Unique categories** | 30+ |
| **Compute shaders** | 27 |

### All Vulkan-Accelerated Nodes (Quick Reference)

| # | Node | Category | Shader |
|---|------|----------|--------|
| 1 | Abs | Math/Base | `abs.comp` |
| 2 | Blend | Operator/Blend | `combiner_blend.comp` |
| 3 | Clamp | Filter/Range | `clamp.comp` |
| 4 | CombinerAdd | Operator/Combiner | `combiner_add.comp` |
| 5 | CombinerDivide | Operator/Combiner | `combiner_divide.comp` |
| 6 | CombinerMax | Operator/Combiner | `combiner_max.comp` |
| 7 | CombinerMin | Operator/Combiner | `combiner_min.comp` |
| 8 | CombinerMultiply | Operator/Combiner | `combiner_multiply.comp` |
| 9 | CombinerSubtract | Operator/Combiner | `combiner_subtract.comp` |
| 10 | Cos | Math/Base | `cos.comp` |
| 11 | Fold | Filter/Recast | `fold.comp` |
| 12 | GaborWaveFbm | Primitive/Coherent | `gabor_wave_fbm.comp` |
| 13 | Gain | Filter/Recurve | `gain.comp` |
| 14 | GammaCorrection | Filter/Recurve | `gamma_correction.comp` |
| 15 | GaussianDecay | Math/Base | `gaussian_decay.comp` |
| 16 | HeightmapToNormalMap | Converter | `normal_map.comp` |
| 17 | HydraulicBlur | Erosion/Hydraulic (WIP) | `hydraulic_blur.comp` |
| 18 | HydraulicStream | Erosion/Hydraulic (WIP) | `hydraulic_erosion.comp` |
| 19 | Inverse | Math/Base | `inverse.comp` |
| 20 | NoiseFbm | Primitive/Coherent | `noise_fbm.comp` |
| 21 | Remap | Filter/Range | `remap.comp` |
| 22 | Rescale | Filter/Range | `rescale.comp` |
| 23 | Saturate | Filter/Recurve | `saturate.comp` |
| 24 | SelectSlope | Mask/Selector | `select_slope.comp` |
| 25 | ShiftElevation | Filter/Range | `shift_elevation.comp` |
| 26 | Smoothstep | Filter/Smoothing | `smoothstep.comp` |

---

## How to Add a New Node

1. **Create** `Hesiod/src/model/nodes/nodes_function/my_node.cpp` with `setup_my_node_node()` and `compute_my_node_node()`
2. **Declare** in `Hesiod/include/hesiod/model/nodes/node_factory.hpp`:
   ```cpp
   DECLARE_NODE(my_node)
   // or for Vulkan: DECLARE_NODE_VULKAN(my_node)
   ```
3. **Register** in `node_factory.cpp` — add to `get_node_inventory()` map and the `switch` statement:
   ```cpp
   // In get_node_inventory():
   {"MyNode", "Category/Subcategory"},

   // In node_factory():
   SETUP_NODE(MyNode, my_node);
   // or for Vulkan: SETUP_NODE_VULKAN(MyNode, my_node);
   ```
4. *(Optional)* **Add Vulkan shader** `Hesiod/shaders/my_node.comp` and implement `compute_my_node_node_vulkan()`

---

*Generated from source code analysis of the TerraLith/Hesiod codebase.*
