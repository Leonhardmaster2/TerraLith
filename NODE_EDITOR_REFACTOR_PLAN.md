# TerraLith / Hesiod — Node Editor Refactor Plan

## Table of Contents
1. [Current Architecture Summary](#1-current-architecture-summary)
2. [Technical Debt Inventory](#2-technical-debt-inventory)
3. [UI/UX Modernization Plan](#3-uiux-modernization-plan)
4. [Step-by-Step Implementation](#4-step-by-step-implementation)

---

## 1. Current Architecture Summary

### 1.1 Layer Overview

The node editor is built across three distinct layers:

```
┌─────────────────────────────────────────────────────────┐
│  HESIOD (Application Layer)                             │
│  - MainWindow, ProjectUI, GraphEditorWidget             │
│  - NodeAttributesWidget, NodeSettingsWidget, Viewer3D   │
│  - BaseNode (292 terrain nodes), NodeFactory             │
│  - ProjectModel, GraphManager, GraphNode                │
├─────────────────────────────────────────────────────────┤
│  GNODEGUI (Visual Layer)                                │
│  - GraphViewer (QGraphicsView canvas)                   │
│  - GraphicsNode (QGraphicsRectItem)                     │
│  - GraphicsLink (QGraphicsPathItem)                     │
│  - GraphicsGroup, GraphicsComment                       │
│  - GraphicsNodeGeometry, Style singleton                │
│  - NodeProxy (abstract model-view bridge)               │
├─────────────────────────────────────────────────────────┤
│  GNODE (Backend/Logic Layer)                            │
│  - Graph, Node (abstract), Port (Input<T>/Output<T>)   │
│  - Link, Data<T>, BaseData                              │
│  - Topological sort (Kahn's), dirty-flag propagation   │
│  - ~1,200 LOC, header-rich, template-heavy             │
└─────────────────────────────────────────────────────────┘
```

### 1.2 Data Models

**GNode (Backend)**
- `Node`: Abstract base with `compute()`. Owns a vector of `shared_ptr<Port>`. Has `is_dirty` flag and `p_graph` back-pointer.
- `Port`: Abstract. Two template subclasses — `Input<T>` (holds `weak_ptr` to upstream data) and `Output<T>` (holds `shared_ptr` owning the data).
- `Data<T>`: Type-erased wrapper around any value. Runtime type identification via `typeid(T).name()`.
- `Link`: POD struct `{from, port_from, to, port_to}`. Stored in `Graph::links` vector.
- `Graph`: Map of `string → shared_ptr<Node>`, vector of `Link`. Provides topological sort, incremental update, and full-graph update.

**GNodeGUI (Visual)**
- `GraphicsNode`: `QGraphicsRectItem`. Renders header (category-colored gradient), caption, ports (circles with data-type colors), optional embedded widget, comment text. Drop shadow via `QGraphicsDropShadowEffect`.
- `GraphicsLink`: `QGraphicsPathItem`. Seven path styles (CUBIC, LINEAR, BROKEN_LINE, CIRCUIT, DEPORTED, QUADRATIC, JAGGED). Colored by output data type.
- `GraphicsNodeGeometry`: Computes layout metrics (port positions, caption rect, body/header rects, full dimensions) based on font metrics and style constants.
- `NodeProxy`: Abstract interface bridging GNodeGUI to any model (id, caption, category, port info, data refs). `TypedNodeProxy<T>` template implementation.
- `Style`: Global singleton with nested structs for `Viewer`, `Node`, `Link`, `Group`, `Comment`. Accessed via `GN_STYLE` macro.

**Hesiod (Application)**
- `BaseNode`: Extends `gnode::Node`. Adds attribute map (`map<string, unique_ptr<AbstractAttribute>>`), compute function injection, category, comment, documentation.
- `GraphNode`: Extends `gnode::Graph`. Adds `GraphConfig` (resolution, tiling), node factory, broadcast parameters.
- `GraphNodeWidget`: Extends `gngui::GraphViewer`. Bridges visual events to backend operations. Owns undo stack (snapshot-based).
- `NodeAttributesWidget`: Wraps `attributes::AttributesWidget` for property editing. Toolbar with update, info, backup, reset actions.

### 1.3 Event Flow

**Pan**: Middle-mouse drag on `GraphViewer`. Translates view by mouse delta. Cursor changes to closed hand.

**Zoom**: Mouse wheel on `GraphViewer`. Scale factor 1.2x per tick, centered on mouse position. No min/max limits currently.

**Node Drag**: Left-click on node body (not port). Sets `is_node_dragged=true`, enables `ItemIsMovable`. On move, updates all connected links via O(K) `all_connected_links` set. Also checks `collidingItems()` for auto-wire drop targets.

**Connection Creation**:
1. Left-click on hovered port → `connection_started` callback
2. Temporary `GraphicsLink` follows mouse cursor
3. `sceneEventFilter` on all other nodes checks port compatibility (same PortType = invalid, different data type = invalid)
4. Incompatible ports dim visually (`color_port_not_selectable`, smaller radius)
5. Release on valid port → `connection_finished` → `GraphViewer` creates permanent link
6. Release on empty → `connection_dropped` → application layer can show context menu

### 1.4 Existing Design Vision (docs/imgui_redesign/)

The codebase contains a detailed ImGui-based redesign spec that has NOT yet been implemented:
- **Theme**: Warm dark greys (#19191C to #39393E), teal primary accent (#4396B2), amber secondary (#D59235)
- **Pin Colors**: 7 distinct data-type colors (Heightmap=Teal, Mask=Amber, Texture=Green, Geometry=Purple, Scalar=Grey, Path=Coral, Cloud=SkyBlue)
- **Node Headers**: Colored by category with 9 accent colors
- **Properties Inspector**: Two-column table layout, collapsible sections, color-coded Vec3 editors
- **Dockspace**: Configurable panels (Node Editor, 3D Viewport, Properties, Node Library, Log)
- **Pin UX**: 6px radius (2x default), filled=connected / hollow=disconnected, hover glow

---

## 2. Technical Debt Inventory

### 2.1 GNode (Backend) — Critical Issues

| # | Issue | Severity | Location |
|---|-------|----------|----------|
| B1 | **No type validation at link creation** — `new_link()` connects ports without verifying data types match. Type mismatch only caught at runtime when values accessed. | High | `graph.cpp` |
| B2 | **No cycle detection** — Graph allows circular connections. Topological sort fails silently on cycles. | High | `graph.cpp` |
| B3 | **Raw `p_graph` back-pointer** — Node stores raw `Graph*` that can dangle if Graph destroyed first. | Medium | `node.hpp` |
| B4 | **No port name validation** — Duplicate port names, empty names accepted silently. | Low | `node.hpp` |
| B5 | **No serialization** — Only Graphviz/Mermaid export (topology only). All persistence delegated to Hesiod layer. | Design | `graph.cpp` |

### 2.2 GNodeGUI (Visual) — Critical Issues

| # | Issue | Severity | Location |
|---|-------|----------|----------|
| V1 | **O(N) node lookup by ID** — `get_graphics_node_by_id()` linear-scans all scene items. Degrades with 500+ nodes. | High | `graph_viewer.cpp` |
| V2 | **Quadratic event filter registration** — `add_item()` installs scene event filters between ALL existing nodes. For N nodes: N*(N-1)/2 registrations. | High | `graph_viewer.cpp` |
| V3 | **No zoom limits** — Wheel zoom is unbounded; user can zoom astronomically far in/out. | Medium | `graph_viewer.cpp` |
| V4 | **Per-frame drop shadow rendering** — Every node has `QGraphicsDropShadowEffect` (blur=12). Significant GPU/CPU overhead at scale. | Medium | `graphics_node.cpp` |
| V5 | **Context menu timer hack** — After Ctrl+RClick delete, disables context menu then uses `QTimer::singleShot(200ms)` to re-enable. Code comment: "this is ugly..." | Low | `graph_viewer.cpp` |
| V6 | **Link path recalculation on every 1px move** — No batching or dirty-flag for link geometry updates during drag. | Medium | `graphics_node.cpp` |
| V7 | **Groups don't move children** — Moving a group rectangle does NOT move the nodes inside it. | Medium | `graphics_group.cpp` |
| V8 | **Incomplete undo/redo** — Signals emitted but only delete operations have actual `QUndoCommand` implementations. | Medium | `graph_viewer.cpp` |

### 2.3 Hesiod (Application) — Critical Issues

| # | Issue | Severity | Location |
|---|-------|----------|----------|
| A1 | **Magic string keys for attributes** — `node.get_attr<T>("noise_type")` with no compile-time checking. Typos silently fail. | Medium | All node files |
| A2 | **Global singleton context** — `HSD_CTX` macro for `AppContext` access everywhere. Hidden dependencies, hard to test. | Medium | Throughout |
| A3 | **Mixed paradigms** — Some events use Qt signals, others use `std::function` callbacks. Inconsistent and confusing. | Medium | `BaseNode`, `GraphNode` |
| A4 | **Main-thread computation** — All node `compute()` runs on UI thread. Long operations block interaction. | High | `graph_node.cpp` |
| A5 | **Unsafe widget lifetime** — `node_settings_widget.cpp` has a "TODO fix, unsafe" comment at line 44. Raw pointers to potentially-deleted widgets. | Medium | `node_settings_widget.cpp` |
| A6 | **No copy-buffer persistence** — Clipboard is session-only JSON blob, no validation on paste. | Low | `graph_node_widget.cpp` |

### 2.4 Cross-Layer Debt

| # | Issue | Severity |
|---|-------|----------|
| X1 | **Style system fragmentation** — Three separate style/theme systems: `gngui::Style` singleton, `AppSettings::Colors`, and `StyleSettings`. Colors duplicated and can drift. |  High |
| X2 | **Design spec not implemented** — The `docs/imgui_redesign/` specs define a comprehensive theme and UX overhaul (TerraLith theme, 6px pins, hover glow, collapsible properties, dockspace layout) that is entirely unimplemented. | High |
| X3 | **No automated tests for GUI** — GNodeGUI has a test folder but only basic integration. No visual regression, no interaction tests. | Medium |

---

## 3. UI/UX Modernization Plan

### 3.1 Visual Hierarchy

**Goal**: Distinct, readable nodes with clear information architecture.

**Current Problems**:
- Node headers use a subtle gradient that doesn't strongly differentiate categories
- Caption text is small and white on all nodes — no typographic hierarchy
- Port labels blend with the body; no visual grouping of inputs vs. outputs
- Embedded widgets expand nodes to unpredictable sizes

**Proposed Changes**:
1. **Bold Category Headers**: Full-width colored header bar (as in imgui_redesign spec) with high-contrast caption text. Category color from `StyleSettings::category_color_map`.
2. **Typography Scale**: Header caption at 1.2x base font size, bold. Port labels at 1.0x. Comment text at 0.85x, italic.
3. **Port Visual Separation**: Input ports grouped on left with left-aligned labels; output ports on right with right-aligned labels. Thin horizontal separator line between header and port area.
4. **Consistent Node Width**: Enforce minimum width from `Style::node.width` and cap maximum width to prevent oversized nodes from dominating the canvas.
5. **Computing State**: Animated border pulse or spinner icon (instead of current 50% opacity dimming which makes nodes look broken).

### 3.2 Interactive Feedback

**Goal**: Every interaction gives immediate, clear visual feedback.

**Current Problems**:
- Port hover only changes `is_port_hovered` bool — the visual change is subtle (just a color tint)
- Connection dragging dims incompatible ports but doesn't highlight the valid target
- Selected nodes get a border color change but no other affordance
- Auto-wire drop target (link highlight) uses hardcoded `QColor(100, 200, 255)`

**Proposed Changes**:
1. **Port Hover Glow**: On hover, port circle scales to 1.3x and gains a semi-transparent glow ring (as per imgui_redesign: "hover glow effect for better discoverability").
2. **Connected vs. Disconnected Ports**: Filled circle = connected, hollow circle = disconnected (per imgui_redesign spec). Currently both are filled.
3. **Active Connection Path**: During drag, valid target ports pulse/glow with the data-type color. Invalid ports fade to 30% opacity (currently they just shrink).
4. **Selected Node Elevation**: Selected nodes get increased drop shadow (blur 20, offset 4,4) and a 2px accent-colored border.
5. **Link Hover Preview**: On hover over a link, show a tooltip with source→target node names and data type.
6. **Auto-Wire Preview**: When dragging a node over a link, show a ghosted preview of how the node would be inserted.

### 3.3 Spatial UX

**Goal**: Smooth, predictable canvas navigation with sensible defaults.

**Current Problems**:
- No zoom limits — can zoom to infinity or to zero
- Pan is middle-mouse only — no spacebar+drag alternative
- No grid or alignment aids
- `compute_graph_layout_sugiyama()` exists but layout quality is unknown
- Groups don't actually contain/move their child nodes
- Default scene range is ±40,000 units with no viewport constraints

**Proposed Changes**:
1. **Zoom Clamping**: Min scale 0.1x, max scale 5.0x. Smooth easing near limits.
2. **Alternative Pan**: Spacebar+left-drag as pan alternative (common in creative tools).
3. **Snap-to-Grid (Optional)**: Configurable grid (default 20px). Nodes snap on release when grid is active. Toggle via Ctrl+G or toolbar.
4. **Smart Auto-Layout**: Improve Sugiyama layout by adding post-processing: even spacing, minimum gap between nodes, alignment of connected chains.
5. **Group-Node Containment**: When a group is moved, all nodes whose center falls within the group rect move with it. Double-click group to "enter" (isolate) its contents.
6. **Minimap**: Small overview widget in the corner showing the full graph with a viewport indicator. Click to navigate.
7. **Zoom-to-Selection**: Ctrl+F to frame selected nodes (or all nodes if none selected).

### 3.4 Architecture Decoupling

**Goal**: Clean separation between node business logic and visual presentation.

**Current State**: The `NodeProxy` interface already provides partial decoupling, but:
- `GraphicsNode` directly accesses `NodeProxy` methods in `paint()`
- Style is a global singleton tightly coupled to rendering
- Event handling mixes view concerns (hover states) with model concerns (connection validation)
- Hesiod's `GraphNodeWidget` extends `GraphViewer`, mixing application logic with canvas rendering

**Proposed Changes**:
1. **Consolidate Style Systems**: Merge `gngui::Style`, `AppSettings::Colors`, and `StyleSettings` into a single `ThemeProvider` interface. GNodeGUI receives theme via dependency injection, not a global singleton.
2. **Extract Connection Validator**: Move port compatibility logic out of `sceneEventFilter` into a separate `ConnectionPolicy` class that GNodeGUI queries. This allows Hesiod to define custom validation rules (e.g., allow certain type coercions).
3. **Node ID Index**: Replace linear scene scans with a `std::unordered_map<string, GraphicsNode*>` maintained by `GraphViewer`.
4. **Event Filter Consolidation**: Replace N^2 per-node event filters with a single `GraphScene` subclass that dispatches port-hover events via spatial lookup.
5. **Render Cache**: Implement `QGraphicsItem::setCacheMode(DeviceCoordinateCache)` for nodes that haven't changed. Invalidate cache only on state change (selection, hover, compute).

---

## 4. Step-by-Step Implementation

Each step is designed to be independently testable and committable.

---

### Step 1: Node ID Index & Zoom Limits (Performance + Safety Foundation)
**Files**: `graph_viewer.hpp`, `graph_viewer.cpp`
**Risk**: Low — additive changes with no visual regressions

**Changes**:
- Add `std::unordered_map<std::string, GraphicsNode*> node_index_` to `GraphViewer`
- Update `add_node()` to insert into index; `delete_graphics_node()` to remove
- Replace all `get_graphics_node_by_id()` linear scans with O(1) index lookups
- Add zoom clamping in `wheelEvent()`: min scale 0.1, max scale 5.0
- Add `zoom_to_selection()` method (Ctrl+F shortcut)

**Tests**:
- Verify node lookup returns correct node after add/delete cycles
- Verify zoom does not exceed limits after rapid wheel events
- Verify zoom-to-selection frames selected nodes with margin

---

### Step 2: Port Visual States (Connected/Disconnected + Hover Glow)
**Files**: `graphics_node.cpp`, `graphics_node_geometry.cpp`, `style.hpp`
**Risk**: Medium — changes paint logic but no interaction changes

**Changes**:
- In `GraphicsNode::paint()`, render ports as:
  - **Connected**: Filled circle (current behavior)
  - **Disconnected**: Hollow circle (2px stroke, no fill)
- Add hover glow effect: when `is_port_hovered[k]`, draw a semi-transparent circle at 1.5x radius behind the port circle
- Increase default `port_radius` from 7.5 to 9.0 (closer to imgui_redesign's 6px ImGui units ≈ 9px Qt)
- Add `port_radius_hovered` to `Style::Node` (default: 12.0)

**Tests**:
- Visual: Screenshot comparison of connected vs. disconnected ports
- Interaction: Verify hover detection still works with larger port radius
- Verify no visual artifacts when rapidly hovering across ports

---

### Step 3: Node Header Redesign (Visual Hierarchy)
**Files**: `graphics_node.cpp`, `graphics_node_geometry.hpp/.cpp`, `style.hpp`
**Risk**: Medium — changes node dimensions, may affect existing layouts

**Changes**:
- Redesign header rendering:
  - Full-width solid color bar (from `color_category` map) instead of gradient
  - Caption text: bold, 1.2x font size, centered vertically in header
  - Thin 1px separator line between header and body
- Add `header_height` to geometry (fixed ratio of `line_height * 1.5`)
- Update `compute_body_and_header()` to account for new header proportions
- Add subtle bottom-border shadow on header (2px gradient fade)

**Tests**:
- Verify all category colors render correctly for every node type
- Verify node height calculations are still correct (ports don't overlap)
- Verify caption text is readable against all category header colors

---

### Step 4: Connection Path Feedback (Drag Highlighting)
**Files**: `graphics_node.cpp` (sceneEventFilter), `graphics_link.cpp`, `graph_viewer.cpp`
**Risk**: Medium — modifies interaction logic during connection creation

**Changes**:
- During active connection drag:
  - Valid target ports: pulse animation (scale oscillation 1.0→1.3→1.0 over 500ms)
  - Invalid ports: fade to 30% opacity (instead of current smaller radius)
  - Temporary link: render with dashed stroke and data-type color
- On connection completion: brief 200ms "flash" animation on the new link
- Refactor port compatibility check from inline `sceneEventFilter` logic into a standalone `is_port_compatible()` method on `GraphicsNode`

**Tests**:
- Verify only type-compatible ports highlight during drag
- Verify same-direction ports (OUT→OUT, IN→IN) never highlight
- Verify temporary link disappears cleanly on drop (no dangling graphics items)
- Verify already-connected input ports don't highlight as targets

---

### Step 5: Event Filter Consolidation (Performance)
**Files**: `graph_viewer.hpp/.cpp`, `graphics_node.hpp/.cpp`
**Risk**: High — changes fundamental event dispatch mechanism

**Changes**:
- Remove per-node `installSceneEventFilter()` calls from `add_item()`
- Create `GraphScene` subclass of `QGraphicsScene`
- Override `GraphScene::event()` to dispatch port-hover updates:
  - On mouse move during active connection, use `items(pos)` to find node under cursor
  - Call `update_is_port_hovered()` directly on that node
  - Remove `sceneEventFilter()` from `GraphicsNode` entirely
- This reduces O(N^2) filter registrations to O(1) scene-level dispatch

**Tests**:
- Verify connection creation still works (start, drag, complete, cancel)
- Verify port hover highlighting works on all nodes
- Verify port compatibility validation still prevents invalid connections
- Performance: measure event overhead with 100+ nodes before and after

---

### Step 6: Theme Consolidation (Architecture)
**Files**: `style.hpp/.cpp`, `app_settings.hpp`, `style_settings.hpp`, new `theme_provider.hpp`
**Risk**: Medium — touches many files but changes are mechanical (color value swaps)

**Changes**:
- Create `ThemeProvider` interface with methods: `get_node_style()`, `get_link_style()`, `get_viewer_style()`, `get_color_for_data_type()`, `get_color_for_category()`
- Implement `TerraLithTheme` using colors from `docs/imgui_redesign/theme_terralith.hpp`:
  - Background: #1E1E22 (warm dark grey, not pure black)
  - Primary accent: #4396B2 (teal)
  - Secondary accent: #D59235 (amber)
  - Text: #E0E2E8 (primary), #80838D (dim)
- Pass `ThemeProvider*` to `GraphViewer` constructor (dependency injection)
- `GraphViewer` and all `Graphics*` items query theme via stored pointer
- Remove `GN_STYLE` global singleton macro
- Update `StyleSettings` and `AppSettings::Colors` to implement `ThemeProvider`

**Tests**:
- Verify all nodes render with correct category colors from new theme
- Verify all links render with correct data-type colors
- Verify background color matches spec
- Visual regression: compare screenshot before/after theme swap

---

### Step 7: Selected Node Elevation & Link Hover Info
**Files**: `graphics_node.cpp`, `graphics_link.cpp`, `graph_viewer.cpp`
**Risk**: Low — additive visual enhancements

**Changes**:
- Selected nodes: increase drop shadow (blur 20, offset 4,4), raise z-order by 1
- Selected nodes: 2px border in primary accent color (#4396B2)
- Link hover: show `QToolTip` with "SourceNode.output → TargetNode.input (DataType)"
- Link hover: thicken stroke to `pen_width_hovered` and apply glow color

**Tests**:
- Verify selected node visually "pops" above unselected neighbors
- Verify multi-select shows elevation on all selected nodes
- Verify tooltip appears after 500ms hover delay on links
- Verify tooltip disappears when mouse leaves link

---

### Step 8: Spacebar Pan & Snap-to-Grid
**Files**: `graph_viewer.hpp/.cpp`
**Risk**: Low — additive input handling

**Changes**:
- Add `keyPressEvent`/`keyReleaseEvent` handlers for spacebar:
  - Space down: set `is_space_pan_mode=true`, cursor changes to open hand
  - Space+left-drag: pan (same as current middle-mouse)
  - Space up: restore previous cursor
- Optional snap-to-grid:
  - Add `bool grid_enabled` and `float grid_size` to viewer state
  - In `drawBackground()`, render dot grid when enabled
  - On node release (`mouseReleaseEvent` after drag), snap position to nearest grid point
  - Toggle via keyboard shortcut (Ctrl+Shift+G) and toolbar button

**Tests**:
- Verify spacebar+drag pans the view
- Verify spacebar doesn't conflict with text input in embedded widgets
- Verify grid renders at correct density across zoom levels
- Verify node snap positions are correct at various zoom levels

---

### Step 9: Group-Node Containment
**Files**: `graphics_group.hpp/.cpp`, `graph_viewer.cpp`
**Risk**: Medium — changes group behavior

**Changes**:
- Override `GraphicsGroup::itemChange()` for `ItemPositionHasChanged`:
  - Find all `GraphicsNode` items whose center is inside the group rect
  - Move them by the same delta as the group
- Add "enter group" on double-click: temporarily hide all nodes outside the group, zoom to fit group contents
- Add "exit group" button or Escape key to return to full view

**Tests**:
- Verify moving a group moves contained nodes
- Verify nodes near the edge (center outside) are NOT moved
- Verify undo/redo works correctly with group moves
- Verify enter/exit group preserves all node positions

---

### Step 10: Computing State Animation & Minimap
**Files**: `graphics_node.cpp`, `graph_viewer.hpp/.cpp`
**Risk**: Low — additive visual features

**Changes**:
- Replace 50% opacity computing indicator with:
  - Animated border: cycling accent color with `QTimeLine` (1s period)
  - Small spinner icon in the header
- Minimap widget:
  - `QGraphicsView` subclass showing the full scene at tiny scale
  - Semi-transparent viewport rectangle overlay
  - Click to pan main view to that location
  - Drag viewport rect to scroll
  - Position: bottom-right corner of `GraphViewer`

**Tests**:
- Verify computing animation starts/stops with `is_node_computing` flag
- Verify minimap reflects current scene contents
- Verify clicking minimap navigates main view correctly
- Verify minimap viewport rect tracks main view position during pan/zoom

---

## Appendix A: File Map (Key Source Locations)

### GNode (Backend)
| File | Purpose |
|------|---------|
| `external/GNode/GNode/include/gnode/node.hpp` | Node base class |
| `external/GNode/GNode/include/gnode/port.hpp` | Port templates (Input/Output) |
| `external/GNode/GNode/include/gnode/graph.hpp` | Graph container |
| `external/GNode/GNode/include/gnode/link.hpp` | Link struct |
| `external/GNode/GNode/include/gnode/data.hpp` | Data<T> wrapper |
| `external/GNode/GNode/src/graph.cpp` | Graph implementation (~500 LOC) |
| `external/GNode/GNode/src/node.cpp` | Node implementation (~134 LOC) |

### GNodeGUI (Visual)
| File | Purpose |
|------|---------|
| `external/GNodeGUI/GNodeGUI/include/gnodegui/graph_viewer.hpp` | Canvas widget |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/graphics_node.hpp` | Visual node item |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/graphics_link.hpp` | Visual link item |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/graphics_group.hpp` | Visual group item |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/graphics_node_geometry.hpp` | Node layout math |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/style.hpp` | Global style singleton |
| `external/GNodeGUI/GNodeGUI/include/gnodegui/node_proxy.hpp` | Model-view bridge |
| `external/GNodeGUI/GNodeGUI/src/graph_viewer.cpp` | Canvas implementation |
| `external/GNodeGUI/GNodeGUI/src/graphics_node.cpp` | Node paint + events |
| `external/GNodeGUI/GNodeGUI/src/graphics_link.cpp` | Link rendering |
| `external/GNodeGUI/GNodeGUI/src/graphics_node_geometry.cpp` | Geometry calculation |
| `external/GNodeGUI/GNodeGUI/src/style.cpp` | Style defaults |

### Hesiod (Application)
| File | Purpose |
|------|---------|
| `Hesiod/include/hesiod/gui/widgets/graph_node_widget.hpp` | Extends GraphViewer |
| `Hesiod/include/hesiod/gui/widgets/node_attributes_widget.hpp` | Property editor |
| `Hesiod/include/hesiod/gui/widgets/node_settings_widget.hpp` | Selected node panel |
| `Hesiod/include/hesiod/gui/widgets/main_window.hpp` | Top-level window |
| `Hesiod/include/hesiod/gui/project_ui.hpp` | Project UI orchestrator |
| `Hesiod/include/hesiod/model/nodes/base_node.hpp` | Base node class |
| `Hesiod/include/hesiod/model/nodes/node_factory.hpp` | Node registration |
| `Hesiod/include/hesiod/model/graph/graph_node.hpp` | Backend graph wrapper |
| `Hesiod/include/hesiod/app/style_settings.hpp` | Category/data colors |
| `Hesiod/include/hesiod/app/app_settings.hpp` | App settings + colors |

### Design Specs (Reference Only)
| File | Purpose |
|------|---------|
| `docs/imgui_redesign/theme_terralith.hpp` | Color palette spec |
| `docs/imgui_redesign/node_ux_and_properties.hpp` | Pin & node UX spec |
| `docs/imgui_redesign/dockspace_layout.hpp` | Layout architecture spec |

## Appendix B: Dependency Order

Steps should be executed in order. Key dependencies:

```
Step 1 (Node Index + Zoom)
  ↓
Step 2 (Port Visuals)  ←  independent of Step 1, but builds on style
  ↓
Step 3 (Header Redesign)  ←  depends on geometry from Step 2
  ↓
Step 4 (Connection Feedback)  ←  depends on port visuals from Step 2
  ↓
Step 5 (Event Filter)  ←  depends on connection logic from Step 4
  ↓
Step 6 (Theme Consolidation)  ←  can happen after Steps 1-5 stable
  ↓
Step 7 (Selection + Link Hover)  ←  uses theme from Step 6
  ↓
Step 8 (Pan + Grid)  ←  independent, can be parallelized with Step 7
  ↓
Step 9 (Group Containment)  ←  independent, can be parallelized with Step 8
  ↓
Step 10 (Animations + Minimap)  ←  final polish, depends on all above
```

## Appendix C: Risk Assessment

| Step | Risk | Mitigation |
|------|------|------------|
| 1 | Low | Pure additions, no behavior change |
| 2 | Medium | Paint-only changes, test with diverse node types |
| 3 | Medium | Changes geometry — verify all 292 node types render correctly |
| 4 | Medium | Interaction changes — thorough manual testing of connection drag |
| 5 | **High** | Replaces fundamental event dispatch — extensive before/after testing |
| 6 | Medium | Touches many files — mechanical but wide blast radius |
| 7 | Low | Additive visual features |
| 8 | Low | Additive input handling |
| 9 | Medium | Changes group semantics — test with existing projects |
| 10 | Low | Additive polish features |
