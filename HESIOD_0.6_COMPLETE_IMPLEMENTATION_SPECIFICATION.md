# TerraLith / Hesiod 0.6 — Complete Implementation Specification

> **Date**: February 26 2026  
> **Version Target**: 0.6.0 (incremental production release)  
> **Purpose**: This is the **single, self-contained, ultra-detailed specification** that must be used to update the entire Hesiod codebase to version 0.6.  
> **Instruction to Claude (or any implementing AI)**: Read this document **in full**. Implement **every single item** exactly as described. Do not add, remove, or reinterpret any feature. Keep full backward compatibility with 0.5.1 projects. Prioritize stability, performance, and visual polish. After implementation, regenerate NODE_REFERENCE.md and APPLICATION_FEATURES.md to reflect 0.6.

This document consolidates **all prior discussion** on node editor polish, new geological primitives, smart preview caching, Vulkan strategy, terminal logging, bug fixes, and new default settings.

---

## Table of Contents

- [Vision & Scope](#vision--scope)
- [Core Architecture Updates](#core-architecture-updates)
- [Application Settings (Expanded for 0.6)](#application-settings-expanded-for-06)
- [Node Editor Visual & UX Overhaul](#node-editor-visual--ux-overhaul)
- [Node Search Improvements](#node-search-improvements)
- [Smart Caching & Preview System (Critical Fix)](#smart-caching--preview-system-critical-fix)
- [Vulkan Acceleration Strategy & Decisions](#vulkan-acceleration-strategy--decisions)
- [Expanded Primitive & Geological Nodes](#expanded-primitive--geological-nodes)
- [Implementation Tips & Tricks for Complex New Nodes](#implementation-tips--tricks-for-complex-new-nodes)
- [Improved Terminal / Console Logging](#improved-terminal--console-logging)
- [Critical Bug Fixes & Stability](#critical-bug-fixes--stability)
- [Other Polish & Performance Wins](#other-polish--performance-wins)
- [Node Inventory Summary (0.6)](#node-inventory-summary-06)
- [Build & Configuration Changes](#build--configuration-changes)
- [Migration Guide (0.5.1 → 0.6)](#migration-guide-051--06)
- [Implementation Priority Order](#implementation-priority-order)

---

## Vision & Scope

Hesiod 0.6 is a **focused, high-polish incremental release** (not 2.0). It makes the editor feel beautiful, fast, and perfectly responsive while adding powerful new geological primitives and fixing daily friction points.

**Explicitly in 0.6**:
- Premium node editor visuals
- 8 new high-impact primitive/geological nodes
- Smart full-graph preview cache (instant Gaea-like node state on click)
- Rich color-coded terminal logging with Vulkan timings & stutter detection
- Shadow resolution crash fix
- Expanded Application Settings panel with sensible defaults
- Fuzzy search improvements

**Explicitly excluded** (0.7+):
- Timeline/keyframe animation
- Compound/subgraph system
- Procedural asset library & scattering
- Python plugins

**Target**: 300 total nodes, 38 Vulkan-accelerated, zero crashes on common operations, sub-second previews.

---

## Core Architecture Updates

- Introduce thin `HesiodCore` namespace (in `core/`) containing:
  - `PreviewCacheManager`
  - `TerminalLogger`
  - `SettingsManager` (new unified JSON-backed settings)
- All changes are additive; no existing classes are modified except for small extensions (e.g. `BaseNode::get_preview_data()`).
- Project `.hsd` format remains v1 (auto-upgrade stub added).

---

## Application Settings (Expanded for 0.6)

New **Settings** dialog (File → Application Settings) with 6 tabs. All settings persisted to `~/.config/hesiod/settings.json` (with defaults below). UI uses Qt widgets matching existing style.

### Interface Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Enable node body previews | true | bool | Show live thumbnail in node |
| Preview type | Terrain (hillshade) | enum | Gray / Magma / Terrain / Histogram |
| Preview resolution | 256x256 | enum | 128 / 256 / 512 |
| Node editor grid style | Blueprint subtle | enum | None / Classic / Blueprint |
| Show category icons in headers | true | bool | SVG icons in node headers |

### Performance Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Enable smart preview cache | true | bool | Instant node state on click (critical) |
| Cache memory limit (MB) | 512 | int | LRU limit |
| Enable incremental evaluation | true | bool | Dirty-flag propagation |
| Default resolution | 2048 | enum | 1024 / 2048 / 4096 / 8192 |
| Default tiling | 4x4 | enum | 2x2 / 4x4 / 8x8 |

### Vulkan Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Enable Vulkan globally | true | bool | Master toggle |
| Fallback to CPU on error | true | bool | Auto-retry on Vulkan failure |
| Vulkan device selection | Auto | string | Dropdown of available devices |

### Logging Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Terminal logging level | Info | enum | Silent / Warning / Info / Debug / Verbose |
| Log Vulkan timings | true | bool | Per-node ms in console |
| Show stutter warnings | true | bool | >150 ms node = yellow warning |

### Node Editor Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Node rounding radius (px) | 8 | int | 0–16 |
| Port size (px) | 22 | int | Hit area |
| Fuzzy search aliases enabled | true | bool | “mtn”, “tree”, etc. |
| Duplicate offset (px) | 220 | int | Horizontal shift on Ctrl+D |

### Viewer Tab
| Setting | Default | Type | Description |
|---------|---------|------|-------------|
| Default shadow resolution | 2048 | enum | 1024 / 2048 / 4096 / 8192 (prevents crash) |
| MSAA level | 4x | enum | Off / 2x / 4x / 8x |

**Implementation note**: `SettingsManager` loads at startup, applies immediately, and broadcasts `settingsChanged` signal. All new settings have tooltips matching the table descriptions.

---

## Node Editor Visual & UX Overhaul

Exact visual spec (apply to `GraphicsNode` and `GraphViewer`):

- **Header**: Linear gradient (top #2A2A2A → bottom category color), 4 px inner shadow, 14 px Inter font bold, 16 px category SVG icon (left-aligned, 20 px).
- **Body**: 8 px rounded corners, 4 px soft drop-shadow (QGraphicsDropShadowEffect), 1 px #444 border, hover glow (#5E81AC 0.3 opacity).
- **Ports**: 22 px diameter, glowing ring (animated scale 1.0→1.15 when dragging link), color per type (Heightmap = #4396B2).
- **Background**: `#1A1A1A`, optional low-opacity blueprint grid (every 64 px), toggleable.
- **Computation feedback**: Soft blue pulse (200 ms) + circular progress ring in top-right corner of node.
- **Groups**: Resizable title bar with color tag, collapse button, live thumbnail when collapsed.

---

## Node Search Improvements

- Trigger: `Ctrl+Space` or double right-click on empty canvas.
- Fuzzy ranking: exact match > recent usage (per-project + global) > category affinity.
- Aliases: “mtn” → AdvancedMountainRange, “tree” → TreePlacement, “ridge” → SelectRidge, “lava” → LavaFlowField.
- Results grouped by category, keyboard navigation (↑↓ Enter), instant insert.

---

## Smart Caching & Preview System (Critical Fix)

**Exact behavior required** (fixes “recompute on every click”):

- After any graph change, `PreviewCacheManager` computes **every node’s output** in topological order (background thread).
- On node selection / viewer pin:
  - Load cached `Heightmap` / `Cloud` / `Path` instantly (0–5 ms).
  - 3D viewport always shows final pinned/export node live.
- Invalidation: only upstream dirty nodes + downstream chain.
- Right-click node → “Force Refresh Preview” forces single-node recompute + cache update.
- Memory: LRU map, max 512 MB (configurable), zstd compression for disk fallback (`~/.hesiod_cache/`).
- Status bar: “Cache 94 % • Preview 3 ms”.

---

## Vulkan Acceleration Strategy & Decisions

**Rule**: Math/noise/blend/filter = Vulkan. Sequential/irregular = CPU.

| Node                     | Vulkan | Reason |
|--------------------------|--------|--------|
| AdvancedMountainRange    | Yes    | Noise + FBM layers |
| TreePlacement            | No     | Point clustering |
| GlacierFormation         | No     | Flow accumulation |
| KarstTerrain             | No     | Dissolution |
| LavaFlowField            | Partial| Flow = Vulkan, crust = CPU post |
| FoothillsTransition      | Yes    | Noise + blend |
| StratifiedErosion        | Yes    | Layer math |
| AlpinePeaks              | Yes    | Ridging noise |

Total Vulkan nodes after 0.6: **38**.

---

## Expanded Primitive & Geological Nodes

Add exactly these 8 nodes (full `setup_` and `compute_` functions required).

| Node                     | Vulkan | Category              | Key Attributes (must expose) |
|--------------------------|--------|-----------------------|------------------------------|
| **AdvancedMountainRange**| Yes    | Primitive/Geological  | style (enum: Alpine/Stratified/...), ridgeDensity, peakSharpness, strataFreq, strataThickness, tectonicTilt, erosionIntensity, seed |
| **TreePlacement**        | No     | Primitive/Geological  | density, minElevation, maxElevation, maxSlope, speciesVariation, clusteringScale, windSway, seed |
| **GlacierFormation**     | No     | Primitive/Geological  | flowStrength, crevasseDensity, moraineHeight, iceTint, seed |
| **KarstTerrain**         | No     | Primitive/Geological  | sinkholeDensity, dissolutionDepth, ridgeJaggedness, seed |
| **LavaFlowField**        | Partial| Primitive/Geological  | flowSpeed, coolingRate, channelWidth, crustThickness, seed |
| **FoothillsTransition**  | Yes    | Primitive/Geological  | transitionWidth, rollFrequency, alluvialStrength, seed |
| **StratifiedErosion**    | Yes    | Erosion/Stratify      | layerCount, hardness[8], erosionRate, seed |
| **AlpinePeaks**          | Yes    | Primitive/Geological  | peakCount, sharpness, baseElevation, seed |

All new nodes must call `post_process_heightmap(node, h)` and support standard post-process attributes.

---

## Implementation Tips & Tricks for Complex New Nodes

**TreePlacement** (most important):
1. Generate density map = SelectSlope(<35°) + SelectInterval(tree-line) + NoiseFbm(low freq).
2. Use `CloudRandomDensity` internally + mask.
3. Output Cloud + optional displacement Heightmap (small Gaussian blur).
4. Pro: Chain with SelectAspect for realistic north/south bias.
5. Performance: cap points at 80k for viewport; higher for bake.

**AdvancedMountainRange**:
- Alpine: high peakSharpness + StratifiedErosion downstream.
- Layer 3 instances with different seeds and Blend(Overlay).

---

## Improved Terminal / Console Logging

Use spdlog with custom sink. Example output required:


[18:42:31.284] [INFO] Graph compute started (2048×2048, 4×4)
[18:42:31.291] [VULKAN] AdvancedMountainRange → 47 ms (hit)
[18:42:31.312] [VULKAN] FoothillsTransition → 19 ms (hit)
[18:42:31.345] [CPU] TreePlacement → 184 ms (OpenMP 8)
[18:42:31.347] [WARN] Stutter: HydraulicStream 312 ms — consider lower iterations
[18:42:31.352] [INFO] Graph complete 428 ms (cache 91 %)

Verbose mode: add Vulkan queue stats. Debug builds always show full log.

---

## Critical Bug Fixes & Stability

1. **Shadow resolution crash**: On resolution change, call `glDeleteFramebuffers`, recreate FBO and texture, reset Vulkan descriptor sets.
2. Preview cache prevents recompute on node click.
3. Memory leak in Cloud/Path repeated ops fixed.
4. Vulkan error messages now printed to terminal.

---

## Other Polish & Performance Wins

- Node menu population 40 % faster (pre-sort vector).
- Topological sort optimized (std::vector + Kahn’s algorithm).
- 4× MSAA default in viewer.
- Launch time -35 % (lazy example loading).

---

## Node Inventory Summary (0.6)

- Total nodes: **300** (292 + 8)
- Vulkan-accelerated: **38**
- Primitive/Geological is now the strongest category in any terrain tool.

---

## Build & Configuration Changes

New CMake options (default ON):
- `HESIOD_ENABLE_SMART_PREVIEW_CACHE`
- `HESIOD_ENABLE_TERMINAL_LOGGING`
- `HESIOD_ENABLE_NODE_EDITOR_POLISH`

---

## Migration Guide (0.5.1 → 0.6)

- Projects open unchanged.
- First run creates `~/.hesiod_cache/` and `settings.json`.
- Old Vulkan nodes keep behavior.

---

## Implementation Priority Order (strict)

1. Application Settings + SettingsManager
2. Smart PreviewCacheManager + instant node previews
3. Node Editor visual overhaul (headers, ports, shadows)
4. Terminal logging + stutter detection
5. Shadow crash fix
6. The 8 new nodes (in order listed in table)
7. Fuzzy search + aliases
8. Polish & performance tweaks
9. Update documentation files
10. Test suite (manual + automated graph load/save)

**Claude**: Implement exactly this specification. Do not deviate. Produce clean, commented C++ code following existing style. After completion, the application must feel dramatically more professional and responsive. If you think that you have achieved all of these changes sucessfully, delete the file.  

*End of specification — February 26 2026*
