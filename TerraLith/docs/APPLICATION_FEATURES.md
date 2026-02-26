# TerraLith / Hesiod — Complete Application Features Reference

> Comprehensive documentation of every feature, UI component, viewport function, and extensibility point in the Hesiod terrain generation application.

---

## Table of Contents

- [Application Overview](#application-overview)
- [Technology Stack](#technology-stack)
- [Main Window Layout](#main-window-layout)
- [Menu Bar](#menu-bar)
- [Node Editor](#node-editor)
- [3D Viewport / Terrain Viewer](#3d-viewport--terrain-viewer)
- [2D Viewer Mode](#2d-viewer-mode)
- [Node Settings Panel](#node-settings-panel)
- [Data Preview System](#data-preview-system)
- [Graph Management](#graph-management)
- [Graph Layout Manager](#graph-layout-manager)
- [Coordinate Frame System](#coordinate-frame-system)
- [Project System](#project-system)
- [Bake and Export System](#bake-and-export-system)
- [Texture Downloader](#texture-downloader)
- [Application Settings](#application-settings)
- [Style and Theming](#style-and-theming)
- [Background Compute / Threading](#background-compute--threading)
- [Undo / Redo System](#undo--redo-system)
- [CLI / Batch Mode](#cli--batch-mode)
- [GPU Acceleration](#gpu-acceleration)
- [Serialization Format](#serialization-format)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Port Types and Color Coding](#port-types-and-color-coding)
- [Category Color Coding](#category-color-coding)
- [Enumerated Options Reference](#enumerated-options-reference)
- [Build Configuration and Feature Flags](#build-configuration-and-feature-flags)
- [External Libraries](#external-libraries)

---

## Application Overview

Hesiod (TerraLith) is a **node-based procedural terrain generation** desktop application. It provides a visual graph editor where users connect processing nodes to create complex terrain heightmaps, textures, and 3D assets — all with real-time preview.

Key capabilities:
- 292 terrain processing nodes across 30+ categories
- Real-time 3D terrain visualization with OpenGL
- Dual GPU acceleration: OpenCL (tile-level) and Vulkan (compute shaders)
- Multi-graph architecture with inter-graph data broadcasting
- High-resolution bake and export pipeline (up to 32768x32768)
- Full project serialization (`.hsd` JSON format)
- CLI batch mode for headless rendering
- Cross-platform: Linux, macOS, Windows

---

## Technology Stack

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **Language** | C++20 | Core application |
| **GUI Framework** | Qt6 (Widgets, OpenGL, Network) | User interface |
| **3D Rendering** | OpenGL 3.3 Core | Terrain visualization |
| **Overlay UI** | Dear ImGui (via GLFW backend) | In-viewport parameter controls |
| **GPU Compute** | Vulkan (compute shaders, optional) | Accelerated node processing |
| **GPU Compute** | OpenCL 1.2 (optional) | Tile-level parallel processing |
| **Math** | GLM | Vector/matrix math for rendering |
| **Math** | GSL (GNU Scientific Library) | Scientific computing, spline interpolation |
| **Image Processing** | OpenCV | Import/export, image operations |
| **3D Import** | Assimp | 3D model loading for export |
| **Parallel** | OpenMP | Multi-threaded terrain generation |
| **Serialization** | nlohmann/json | Project save/load |
| **Logging** | spdlog | Application logging |
| **Noise** | FastNoiseLite | Core noise generation |
| **Triangulation** | Delaunator | Delaunay mesh generation |
| **Clustering** | DKM | K-means clustering |
| **CLI Parsing** | args.hxx | Command-line arguments |

---

## Main Window Layout

The main window (`MainWindow`) is a `QMainWindow` with this layout:

```
+--------------------------------------------------------------+
| Menu Bar: [Icon] | File | Project | Graph | View             |
+--------------------------------------------------------------+
| 3D Viewer Dock (Top, movable, closable)                      |
|   ┌──────────────────────────────────────────────┐           |
|   │  OpenGL Terrain Preview  (qtr::RenderWidget) │           |
|   │  + ImGui overlay controls                    │           |
|   └──────────────────────────────────────────────┘           |
+--------------------------------------------+-----------------+
|  Central Widget: Graph Tabs                | Node Settings   |
|   ┌──────────────────────────────────┐     | Dock (Right)    |
|   │ [Tab: main] [Tab: graph_2] ...  │     | ┌─────────────┐ |
|   │                                  │     | │ Attributes  │ |
|   │  Node Editor Canvas              │     | │ for selected│ |
|   │  (GraphNodeWidget)               │     | │ node        │ |
|   │                                  │     | │             │ |
|   │  - Nodes with previews          │     | │ [Auto-Update]│ |
|   │  - Cubic/straight links         │     | │ [Force Build]│ |
|   │  - Comments, groups             │     | └─────────────┘ |
|   └──────────────────────────────────┘     |                 |
+--------------------------------------------+-----------------+
| Status Bar: [Resolution: 1024x1024 ▼]     [██████ Progress] |
+--------------------------------------------------------------+
```

### Dock Widgets
- **3D Viewer Dock** — Top area, movable to any edge, closable via View menu
- **Node Settings Dock** — Right side, movable, closable via View menu
- Both docks automatically swap content when switching between graph tabs

### Status Bar
- **Resolution combo** — Quick global resolution selector (256 to 8192, powers of 2)
- **Progress bar** — Shows graph computation progress

---

## Menu Bar

### [Icon] Menu
| Action | Description |
|--------|-------------|
| Quick Help | Opens HTML quick-start guide in a popup |
| Online Help | Opens external documentation in browser (`hesioddoc.readthedocs.io`) |
| About | Version and credits dialog |

### File Menu
| Action | Shortcut | Description |
|--------|----------|-------------|
| New | `Ctrl+N` | Create a new empty project (with confirmation) |
| Open | `Ctrl+O` | Open a `.hsd` project file |
| Open Ready-made Example | — | Browse and load bundled example projects |
| Save | `Ctrl+S` | Save current project |
| Save As | `Ctrl+Shift+S` | Save to a new file |
| Save a copy | `Ctrl+Alt+S` | Save a timestamped copy without changing current path |
| Bake and Export | `Alt+E` | High-resolution bake and export pipeline |
| Application Settings | — | Open global settings dialog |
| Quit | `Ctrl+Q` | Quit with confirmation |

### Project Menu
| Action | Description |
|--------|-------------|
| Project Settings | Edit project name, comment, and configuration |

### Graph Menu
| Action | Shortcut | Description |
|--------|----------|-------------|
| New graph | — | Add a new graph tab to the project |
| Advance Random Seeds | `Alt+R` | Increment all random seeds across all graphs |
| Reverse Random Seeds | `Alt+Shift+R` | Decrement all random seeds |

### View Menu
| Action | Description |
|--------|-------------|
| Graph Layout Manager | Toggle the multi-graph layout/order manager window |
| Texture Downloader | Toggle the texture downloading panel |
| Show Viewer in Main Window | Toggle the 3D viewer dock visibility |
| Show Node Settings Pan | Toggle the node settings dock visibility |

---

## Node Editor

The node editor canvas (`GraphNodeWidget`, extends `gngui::GraphViewer`) is the primary workspace. It is a `QGraphicsView`-based interactive canvas.

### Canvas Navigation
| Action | Control |
|--------|---------|
| Pan canvas | Middle-mouse drag |
| Zoom in/out | Scroll wheel (range: 0.3x to 5.0x) |
| Zoom to fit all content | Toolbar button or keyboard |
| Zoom to selection | Keyboard shortcut |
| Edge auto-panning | Dragging links near viewport edges auto-scrolls (40px margin) |

### Node Operations
| Action | How |
|--------|-----|
| Create node | Right-click canvas → searchable context menu organized by category |
| Select node | Left-click on node |
| Multi-select | Rubber band selection (click + drag on empty space) |
| Select all | Keyboard shortcut |
| Delete selected | `Delete` key |
| Move node | Drag node with left mouse |
| Duplicate node(s) | Keyboard shortcut (places copies with configurable offset, default 200px) |
| Copy node(s) | Keyboard shortcut → clipboard as JSON |
| Paste node(s) | Keyboard shortcut → places from JSON clipboard |
| Drop node onto link | Auto-wires: the dropped node is inserted inline on the existing link |
| Node info | Right-click context menu → shows runtime info, execution time, backend used |
| Reload node | Right-click or toolbar icon → re-computes single node |

### Connection / Link Operations
| Action | How |
|--------|-----|
| Create link | Click + drag from an output port to an input port |
| Delete link | Select and delete, or drag-disconnect |
| Connection dropped | Dropping a half-drawn link onto empty canvas opens the node creation menu |
| Toggle link style | Button → switches between cubic bezier and straight lines |
| Port compatibility | Animated pulse highlights compatible ports during link dragging |
| Type checking | Connections are type-safe — only matching data types can connect |

### Toolbar Icons
The node editor has a floating toolbar with these actions (visible as icons):
- **New graph** — Create new graph
- **Save** — Save project
- **Load** — Load project
- **Import** — Import graph snippet
- **Select All** — Select all nodes
- **Clear All** — Clear entire graph
- **Fit Content** — Zoom to fit
- **Screenshot** — Save graph screenshot as PNG
- **Link Type** — Toggle cubic/straight links
- **Lock** — Lock/unlock node positions
- **Reload** — Reload/recompute graph
- **Group** — Create a group around selected nodes
- **Dots** — Settings

### Graph-Level Operations
| Action | Description |
|--------|-------------|
| Clear graph | Remove all nodes and links |
| Import graph | Import a graph JSON snippet, placed at cursor position |
| Reload graph | Recompute entire graph from scratch |
| Graph settings | Edit resolution, tiling, overlap for this graph |
| Automatic layout | Auto-arrange nodes in a readable layout (configurable spacing) |
| Export to Graphviz | Export graph structure as `.dot` file for debugging |
| Save screenshot | Export canvas as `.png` image |

### Node Body Features
- **Data preview** — Thumbnail heightmap preview directly in the node body (configurable: Grayscale, Magma, Terrain hillshade, Histogram)
- **Inline settings** — Optional: show attribute widgets directly in the node body (togglable)
- **Category coloring** — Node header color based on its category
- **Port coloring** — Port colors indicate data type (see Port Types section)
- **Progress indicator** — Visual feedback during computation
- **Compute backend indicator** — Shows whether last computation used CPU or Vulkan

### Comments and Groups
- **Comments** — Free-text annotations on the canvas (`GraphicsComment`)
- **Groups** — Visual grouping of nodes with a labeled bounding box (`GraphicsGroup`)

---

## 3D Viewport / Terrain Viewer

The 3D viewer (`Viewer3D` → `qtr::RenderWidget`) provides real-time OpenGL rendering of the terrain.

### Rendering Engine
- **OpenGL 3.3 Core Profile** — Hardware-accelerated rendering
- **ImGui overlay** — In-viewport parameter controls via Dear ImGui
- **Shader-based pipeline** — Custom GLSL shaders managed by `ShaderManager`

### Camera Controls
| Action | Control |
|--------|---------|
| Orbit rotate | Left-mouse drag |
| Pan | Middle-mouse drag (or shift+left) |
| Zoom | Scroll wheel (adjusts orbit distance) |
| Auto-rotate camera | Toggle in ImGui overlay |
| Reset camera | Button in overlay |

### Camera Parameters
- Orbit-based camera: `target`, `distance`, `alpha_x` (pitch), `alpha_y` (yaw)
- Pan offset (`pan_offset`)
- Perspective projection: configurable FOV (default 45 deg), near/far planes (0.01 to 100)

### Render Modes
| Mode | Description |
|------|-------------|
| **3D Render** | Full 3D terrain with lighting, shadows, and textures |
| **2D Render** | Top-down orthographic view with colormap |
| **Wireframe** | Toggle wireframe overlay |

### Scene Components (individually togglable)
| Component | Description |
|-----------|-------------|
| **Heightmap mesh** | The main terrain surface |
| **Ground plane** | Infinite ground reference plane |
| **Water surface** | Animated water with waves, foam, and depth coloring |
| **Points** | Point cloud visualization (instanced rendering) |
| **Path** | Path visualization as line mesh |
| **Rocks** | Instanced rock meshes scattered on terrain |
| **Trees** | Instanced tree meshes placed on terrain |
| **Leaves** | Instanced leaf/foliage meshes |

### Lighting System
| Feature | Controls |
|---------|----------|
| **Directional light** | Azimuth (`light_phi`) and zenith (`light_theta`), configurable distance |
| **Auto-rotate light** | Toggle for animated light orbit |
| **Shadow mapping** | Configurable resolution (default 2048), strength, bypass toggle |
| **Ambient occlusion** | Strength, radius controls |

### Texturing and Materials
| Feature | Controls |
|---------|----------|
| **Albedo texture** | RGBA texture applied to terrain mesh |
| **Normal map** | Per-pixel normal perturbation |
| **Heightmap texture** | Used for displacement and terrain coloring |
| **Bypass albedo** | Toggle to show raw heightmap without texture |
| **Normal visualization** | Toggle to show computed normals |
| **Normal map scaling** | Adjustable normal map intensity |

### Post-Processing Effects
| Effect | Parameters |
|--------|-----------|
| **Gamma correction** | Adjustable gamma value (default 2.0) |
| **Tonemapping** | Toggle HDR tonemapping |
| **Fog** | Color, density, height parameters |
| **Atmospheric scattering** | Rayleigh/Mie color, density, fog/scattering blend ratio |

### Water Rendering
| Feature | Parameters |
|---------|-----------|
| **Water colors** | Shallow and deep water color, depth falloff |
| **Specular** | Water specular highlight strength |
| **Foam** | Color, depth threshold, toggle |
| **Waves** | Direction, frequency (`kw`), amplitude, normal amplitude, speed |
| **Wave animation** | Togglable real-time wave animation |
| **Exclude below** | Water geometry excludes values below threshold |

### Mesh Quality
| Setting | Options |
|---------|---------|
| **Downsample level** | 0 = Full, 1 = 1/2, 2 = 1/4, 3 = 1/8 resolution |
| **Heightmap skirt** | Add a skirt around the terrain edges (configurable) |
| **Shadow map resolution** | 256 to 8192 pixels |

### Viewer-Node Binding
- The viewer auto-detects which ports to display from the selected node
- **Pin node** — Lock the viewer to a specific node even when selecting others
- **Port mapping** — Configurable assignment of node output ports to viewer slots (heightmap, albedo, normal map, water, points, path)
- **Wild guess** — Automatic heuristic port assignment based on node type and data types

### Viewer Serialization
The complete viewer state (camera position, lighting, render toggles, water settings, fog, etc.) is saved/restored with the project.

---

## 2D Viewer Mode

The `RenderWidget` also supports a 2D top-down rendering mode:

| Feature | Description |
|---------|-------------|
| **Orthographic view** | Top-down heightmap display |
| **Colormaps** | Gray, Viridis, Turbo, Magma |
| **Hillshading** | Configurable sun azimuth and zenith |
| **Zoom + Pan** | 2D navigation controls |

---

## Node Settings Panel

The `NodeSettingsWidget` appears in the right dock and displays:

- **Attribute widgets** — All editable parameters for the selected node, rendered as typed widgets (sliders, spinboxes, checkboxes, color pickers, curve editors, cloud editors, path editors, file browsers, etc.)
- **Auto-Update toggle** — When enabled, changing any attribute immediately triggers graph recomputation
- **Force Build button** — Manually trigger a full graph rebuild
- **Pinned nodes** — Multiple nodes can be pinned to show their settings simultaneously

The attribute widgets support groupboxes and separators for organized layout.

---

## Data Preview System

Each node can display a thumbnail preview (`DataPreview`) directly in its body on the canvas.

### Preview Types
| Type | Description |
|------|-------------|
| **Grayscale** | Black-to-white heightmap rendering |
| **Magma** | Magma colormap (perceptually uniform) |
| **Terrain (hillshade)** | Terrain colormap with hillshading |
| **Histogram** | Value distribution histogram |

### Preview Configuration
- Preview resolution: configurable (default 128x128)
- Right-click on preview → switch between preview types
- Can be globally toggled via application settings

---

## Graph Management

### Multi-Graph Architecture
The application supports multiple independent graphs within a single project:
- Each graph is a separate `GraphNode` with its own set of nodes and links
- Graphs are displayed as tabs in the central widget
- Each graph has its own `GraphConfig` (resolution, tiling, overlap)

### GraphManager
The `GraphManager` orchestrates all graphs:
- Add/remove graphs
- Reorder graphs (affects evaluation order)
- Change resolution for all graphs simultaneously
- Coordinate inter-graph broadcasting
- Export flattened output

### Inter-Graph Communication
- **Broadcast nodes** send data to a named tag
- **Receive nodes** in any graph can subscribe to any broadcast tag
- Data is automatically transformed between coordinate frames
- Tag list updates propagate across all graphs in real-time

---

## Graph Layout Manager

The `GraphManagerWidget` is a separate floating window for managing multiple graphs:

| Feature | Description |
|---------|-------------|
| **Graph list** | Reorderable list of all graphs (drag to reorder) |
| **Context menu** | Right-click for graph-specific actions |
| **Add graph** | Create new graph |
| **Remove graph** | Delete a graph (with confirmation) |
| **Apply changes** | Commit reordering changes |
| **Export** | Flatten and export all graphs |
| **Reseed** | Advance/reverse random seeds across all graphs |
| **Coordinate frame widget** | Visual 2D frame layout for multi-graph spatial arrangement |

---

## Coordinate Frame System

Each graph has a `CoordFrame` (origin, size, angle) defining its spatial extent. The `CoordFrameWidget` provides:

- **Visual frame editor** — 2D top-down view showing all graph frames as rectangles
- **Drag to move** — Reposition graph frames
- **Resize handles** — Resize graph extents
- **Rotation handles** — Rotate graph orientation
- **Z-depth ordering** — Layer graphs front-to-back
- **Background image** — Each frame can display a preview thumbnail
- **Mouse wheel zoom** — Zoom the frame overview

---

## Project System

### ProjectModel
| Feature | Description |
|---------|-------------|
| **Name** | Project display name |
| **Path** | File system path (`.hsd`) |
| **Comment** | Free-text project description |
| **Dirty tracking** | Tracks unsaved changes, shown in title bar with `*` |
| **Bake config** | Persisted export settings |
| **Graph manager** | Contains all graphs and their nodes |

### File Format
- Projects are saved as `.hsd` files (JSON)
- Automatic `.bak` backup files on save
- Temporary crash-recovery copies in system temp directory
- UI state (viewer positions, dock layout, scroll positions) saved alongside model state

### Project Settings Dialog
Editable via Project → Project Settings:
- Project name
- Project comment

### Example Projects
- Bundled example projects in `data/bootstraps/`
- Example selector dialog at startup (configurable)
- Ready-made examples accessible via File → Open Ready-made Example

---

## Bake and Export System

The **Bake and Export** pipeline (`Alt+E`) is the primary way to produce final high-resolution outputs.

### Bake Configuration Dialog
| Setting | Options |
|---------|---------|
| **Resolution** | 256 to 32768 (max configurable, default limit 32768) |
| **Output path** | Browse or auto-derive from project path |
| **Base name** | Custom file naming prefix |
| **Format override** | Use node settings, or force PNG8/PNG16/RAW16 |
| **Number of variants** | Generate N+1 variations with incremented random seeds |
| **Force distributed mode** | Use tiled computation for large outputs |
| **Force auto-export** | Override export node settings to always export |
| **Rename export files** | Rename output files to match variant naming |

### Export Process
1. Saves a temporary `.hsd` project file
2. Overrides export node paths and settings
3. Runs the batch pipeline (CLI mode) for each variant
4. Each variant increments random seeds
5. Output organized into `variants_N/` subdirectories
6. Progress dialog blocks UI during export

### Export Formats (for heightmaps)
| Format | Description |
|--------|-------------|
| `PNG (8-bit)` | Standard PNG, 256 gray levels |
| `PNG (16-bit)` | High-precision PNG, 65536 gray levels |
| `RAW (16-bit, Unity)` | Raw binary for Unity terrain import |
| `R16 (16-bit)` | 16-bit raw single-channel |
| `R32 (32-bit float)` | Full floating-point precision |

### Export Node Types
| Node | Exports |
|------|---------|
| ExportHeightmap | Heightmap in chosen format |
| ExportNormalMap | Normal map image |
| ExportTexture | RGBA texture image |
| ExportAsset | 3D mesh + texture asset |
| ExportCloud | Point cloud data |
| ExportCloudToPly | Cloud as PLY mesh |
| ExportPointsToPly | Points as PLY mesh |
| ExportPath | Path data |

---

## Texture Downloader

The `QTextureDownloader` widget provides:
- Async texture downloading from external sources
- Network-based texture browsing and fetching
- Drag-and-drop textures into the node editor
- Automatically creates `ImportTexture` nodes with downloaded files

---

## Application Settings

Accessible via File → Application Settings. Persisted to a JSON config file.

### Model Settings
| Setting | Default | Description |
|---------|---------|-------------|
| Allow broadcast/receive within same graph | `true` | Whether Broadcast↔Receive can work in the same graph |

### Interface Settings
| Setting | Default | Description |
|---------|---------|-------------|
| Data preview in node body | `true` | Show thumbnail previews inside nodes |
| Node settings in node body | `false` | Show attribute widgets inline in nodes |
| Enable texture downloader | `true` | Show texture downloader panel |
| Enable tool tips | `true` | Show hover tooltips |
| Example selector at startup | `true` | Show example project chooser on launch |

### Node Editor Settings
| Setting | Default | Description |
|---------|---------|-------------|
| GPU device name | `""` (auto) | OpenCL device selection |
| Default resolution | `1024` | Default heightmap resolution |
| Default tiling | `4` | Default tile count per axis |
| Default overlap | `0.5` | Default tile overlap |
| Preview size | `128 x 128` | Node preview thumbnail resolution |
| Position delta for duplicate | `200` | Pixel offset when duplicating nodes |
| Auto-layout spacing | `256 x 384` | Node spacing for auto-layout |
| Show node settings panel | `true` | Default visibility of settings dock |
| Show viewer | `true` | Default visibility of 3D viewer dock |
| Max bake resolution | `32768` | Maximum resolution for bake/export |
| Disable during update | `false` | Lock UI during graph computation |
| Enable node groups | `true` | Allow visual node grouping |

### Viewer Settings
| Setting | Default | Description |
|---------|---------|-------------|
| Width | `512` | Viewer default width |
| Height | `512` | Viewer default height |
| Add heightmap skirt | `true` | Add skirt geometry around terrain edges |

### Color Settings
| Color | Hex | Purpose |
|-------|-----|---------|
| `bg_deep` | `#191919` | Deepest background |
| `bg_primary` | `#2B2B2B` | Primary background |
| `bg_secondary` | `#4B4B4B` | Secondary background |
| `text_primary` | `#F4F4F5` | Main text |
| `text_secondary` | `#949495` | Subdued text |
| `text_disabled` | `#3C3C3C` | Disabled text |
| `accent` | `#5E81AC` | Accent color |
| `accent_bw` | `#FFFFFF` | High-contrast accent |
| `border` | `#5B5B5B` | Border color |
| `hover` | `#8B8B8B` | Hover highlight |
| `pressed` | `#ABABAB` | Pressed state |
| `separator` | `#ABABAB` | Separator lines |

### Window Geometry
All window positions and sizes are saved and restored on restart.

---

## Style and Theming

The application uses a custom dark theme applied globally via Qt stylesheets (`apply_global_style()`).

---

## Background Compute / Threading

Graph computation runs on a dedicated background thread to keep the UI responsive.

### GraphWorker
| Feature | Description |
|---------|-------------|
| **Background thread** | `QThread`-based worker for node computation |
| **Ordered execution** | Nodes are computed in topological sort order |
| **Progress reporting** | Per-node progress updates sent to UI |
| **Cancellation** | Atomic cancel flag allows aborting long computations |
| **Backend tracking** | Each node reports whether it used CPU or Vulkan |
| **Execution timing** | Per-node execution time in milliseconds |

### Signals Emitted
| Signal | When |
|--------|------|
| `node_compute_started` | Before each node begins computation |
| `node_compute_finished` | After each node completes |
| `progress_updated` | Percentage progress through the sorted node list |
| `node_execution_time` | Execution time and backend type after each node |
| `compute_all_finished` | All nodes done (includes cancellation flag) |

### UI Integration
- Node bodies show visual "computing" state during background work
- Progress bar in status bar shows overall progress
- "Updating graph..." / "Graph updated successfully." status messages
- Optional: disable entire node editor during updates (`disable_during_update` setting)

---

## Undo / Redo System

The node editor includes a `QUndoStack`-based undo/redo system:

| Feature | Description |
|---------|-------------|
| **Undo** | Revert the last action |
| **Redo** | Re-apply a reverted action |
| **Tracked actions** | Node creation, deletion, connection changes, node movement |
| **Delete with undo** | Deleted nodes can be restored via undo |

---

## CLI / Batch Mode

Hesiod can run without a GUI for automated pipelines.

### Command-Line Interface
```
hesiod [options] [file.hsd]

Options:
  --shape <WxH>      Override heightmap resolution (e.g., 2048x2048)
  --tiling <TxT>     Override tiling (e.g., 4x4)
  --overlap <float>  Override tile overlap (e.g., 0.5)
  --batch            Run in batch mode (compute + export, no GUI)
  --inventory        Dump node inventory to CSV and Mermaid diagram
  --snapshots        Generate attribute screenshot images for documentation
```

### Batch Mode Functions
| Function | Description |
|----------|-------------|
| `run_batch_mode()` | Load project, optionally override resolution/tiling, compute all graphs, trigger exports |
| `run_node_inventory()` | Generate `node_inventory.csv` and `node_inventory.mmd` (Mermaid diagram) |
| `run_snapshot_generation()` | Render PNG screenshots of every node's attribute widget for documentation |

---

## GPU Acceleration

### Dual GPU Strategy
| Backend | Technology | Scope | When Used |
|---------|-----------|-------|-----------|
| **OpenCL** | OpenCL 1.2 | Tile-level parallelism | `GPU` bool attribute on noise nodes, selected per-node |
| **Vulkan** | Vulkan compute shaders | Per-tile GPU dispatch | 26 nodes with `SETUP_NODE_VULKAN`, auto-fallback to CPU |

### OpenCL
- Initialized at application startup
- Falls back gracefully if initialization fails
- Disabled by default on macOS (deprecated API)
- Per-node `GPU` toggle attribute
- Transform modes: `DISTRIBUTED` (tiled CPU), `SINGLE_ARRAY` (GPU)

### Vulkan
- Enabled via `HESIOD_ENABLE_VULKAN` build flag
- 27 GLSL compute shaders compiled to SPIR-V at build time
- Singleton `VulkanContext` manages device, queue, command pool
- `VulkanGenericPipeline` dispatches arbitrary compute with push constants
- Specialized pipelines: `VulkanNoisePipeline`, `VulkanErosionPipeline`
- Per-node toggle: `set_vulkan_enabled()` / `is_vulkan_enabled()`
- Auto-fallback: if Vulkan compute returns `false`, node re-runs on CPU
- Runtime backend tracking: `ComputeBackend::CPU`, `VULKAN`, `OPENCL`, `NONE`

---

## Serialization Format

### `.hsd` File Structure (JSON)
```json
{
  "Hesiod version": "v0.5.1",
  "saved_at": "2025-01-15_14-30-22",
  "project": {
    "name": "my_terrain",
    "comment": "...",
    "bake_config": { ... },
    "graph_manager": {
      "id": "...",
      "graph_order": ["main", "detail"],
      "graph_nodes": {
        "main": {
          "config": { "shape": [1024, 1024], "tiling": [4, 4], "overlap": 0.5 },
          "coord_frame": { ... },
          "nodes": { ... },
          "links": { ... }
        }
      },
      "broadcast_params": { ... },
      "flatten_config": { ... }
    }
  },
  "ui_state": {
    "graph_tabs": { ... },
    "graph_manager_widget": { ... },
    "viewer_states": { ... }
  }
}
```

### What is Serialized
- All node types, positions, attribute values
- All connections/links
- Graph configuration (resolution, tiling, overlap)
- Coordinate frames
- Broadcast tags and mappings
- Bake configuration
- Complete viewer state (camera, lighting, all render settings)
- Window geometry and dock positions
- Node settings panel state

---

## Keyboard Shortcuts

### Global
| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New project |
| `Ctrl+O` | Open project |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Alt+S` | Save a copy |
| `Ctrl+Q` | Quit |
| `Alt+E` | Bake and Export |
| `Alt+R` | Advance random seeds |
| `Alt+Shift+R` | Reverse random seeds |

### Node Editor (handled by `GraphViewer::keyPressEvent`)
| Shortcut | Action |
|----------|--------|
| `Delete` | Delete selected nodes/links |
| `Ctrl+C` | Copy selected nodes |
| `Ctrl+V` | Paste nodes |
| `Ctrl+D` | Duplicate selected nodes |
| `Ctrl+A` | Select all |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` / `Ctrl+Shift+Z` | Redo |
| Right-click on canvas | New node context menu |
| Right-click on node | Node context menu (info, reload, etc.) |

---

## Port Types and Color Coding

Port colors are defined in `StyleSettings::data_color_map`:

| Data Type | Color | Hex |
|-----------|-------|-----|
| **Heightmap** | Teal | `#4396B2` |
| **HeightmapRGBA** | Green | `#50C878` |
| **Array** (Kernel/Mask) | Amber | `#D59235` |
| **Cloud** | Sky Blue | `#87CEEB` |
| **Path** | Coral | `#FF7F7F` |
| **HeightmapVector** | Red | `#FF5555` |
| **ScalarVector** | Grey | `#A0A0A0` |

---

## Category Color Coding

Node header colors based on category (`StyleSettings::category_color_map`):

| Category | Color | Hex |
|----------|-------|-----|
| **Primitive** | Teal | `#2AA198` |
| **Filter** | Purple | `#6C71C4` |
| **Operator** | Purple | `#6C71C4` |
| **Erosion** | Orange | `#CB4B16` |
| **Features** | Yellow | `#B58900` |
| **Hydrology** | Blue | `#268BD2` |
| **Mask** | Pink | `#D33682` |
| **Math** | Dark Teal | `#002B36` |
| **Geometry** | Gray | `#657B83` |
| **Texture** | Black | `#000000` |
| **IO** | Beige | `#CBC4B1` |
| **Routing** | Beige | `#BCB6A3` |
| **Converter** | Beige | `#BCB6A3` |
| **Debug** | Red | `#C80000` |
| **WIP** | White | `#FFFFFF` |

---

## Enumerated Options Reference

These are the configurable enum options available across various nodes:

### Noise Types
Perlin, Perlin (billow), Perlin (half), OpenSimplex2, OpenSimplex2S, Value, Value (cubic), Value (delaunay), Value (linear), Worley, Worley (double), Worley (value)

### Blending Methods
Add, Exclusion, Gradients, Maximum, Maximum (smooth), Minimum, Minimum (smooth), Multiply, Multiply+Add, Negate, Overlay, Replace, Soft, Subtract

### Colormaps
Bone, Gray, Jet, Magma, Nipy Spectral, Terrain, Viridis

### Distance Functions
Chebyshev, Euclidean, Euclidean/Chebyshev, Manhattan

### Kernel Types
Biweight, Cone, Cone Smooth, Cubic Pulse, Disk, Lorentzian, Smooth Cosine, Square, Tricube

### Mask Combine Methods
Union, Intersection, Exclusion

### Voronoi Return Types
F1 squared, F2 squared, F1*F2 squared, F1/F2 squared, F2-F1 squared, Edge distance (exponential), Edge distance (squared), Cell value, Cell value * (F2-F1)

### Periodicity Types
X-only, Y-only, X and Y

### Erosion Profiles
Cosine, Saw Sharp, Saw Smooth, Sharp Valleys, Square Smooth, Triangle Grenier, Triangle Sharp, Triangle Smooth

### Stamping Blend Methods
Add, Maximum, Minimum, Multiply, Subtract

### Clamping Modes
Keep positive & clamp, Keep negative & clamp, Clamp both, No clamping

### Distance Transform Types
Exact, Approx (fast), Manhattan (fast), Jump flooding (GPU)

### Radial Profiles
Gain, Linear, Power Law, Smoothstep, Smoothstep Upper

---

## Build Configuration and Feature Flags

| Flag | Default | Description |
|------|---------|-------------|
| `HESIOD_ENABLE_VULKAN` | `ON` | Vulkan GPU compute support (requires `glslc`) |
| `HESIOD_MINIMAL_NODE_SET` | `OFF` | Build with only core nodes (faster compilation during development) |
| `HESIOD_ENABLE_LTO` | `OFF` | Link-time optimization and dead code elimination |
| `HESIOD_ENABLE_GENERATE_APP_IMAGE` | `ON` | Generate distributable AppImage |
| `HESIOD_UNUSED_FUNCTIONS` | `OFF` | Warn on unused static functions |
| `HSD_DEFAULT_GPU_MODE` | `true` (Linux/Win), `false` (macOS) | Default GPU mode for OpenCL nodes |

---

## External Libraries

| Library | Type | Purpose |
|---------|------|---------|
| **Qt6** | Dynamic | GUI framework (Widgets, OpenGL, Network) |
| **spdlog** | Dynamic | Logging |
| **nlohmann_json** | Header-only | JSON serialization |
| **GLM** | Header-only | Math (vectors, matrices) |
| **GSL** | Dynamic | GNU Scientific Library |
| **OpenCV** | Dynamic | Image processing |
| **Assimp** | Dynamic | 3D model import |
| **OpenMP** | System | Multi-threading |
| **Vulkan** | System (optional) | GPU compute shaders |
| **OpenCL** | System (optional) | GPU tile-level compute |
| **Dear ImGui** | Static | In-viewport GUI overlay |
| **GLFW** | Dynamic | Window management (for ImGui backend) |
| **FastNoiseLite** | Header-only | Noise generation |
| **Delaunator** | Header-only | Delaunay triangulation |
| **DKM** | Header-only | K-means clustering |
| **libnpy** | Header-only | NumPy format I/O |
| **args.hxx** | Header-only | CLI argument parsing |

---

*Generated from source code analysis of the TerraLith/Hesiod codebase (v0.5.1).*
