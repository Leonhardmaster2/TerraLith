#pragma once
// ============================================================================
// TerraLith — ImGui DockSpace Layout
// ============================================================================
// This file provides the full docking infrastructure for the application.
// It sets up a persistent, user-rearrangeable layout with:
//   - A top menu bar
//   - A central Node Editor workspace
//   - A dockable 3D Viewport (default: top-left split)
//   - A right-side Properties Inspector
//   - A bottom Log/Output panel
//
// Requirements: Dear ImGui (Docking branch), imgui_internal.h for DockBuilder
// ============================================================================

#include "imgui.h"
#include "imgui_internal.h" // Required for DockBuilder API

#include <string>

namespace terralith::ui
{

// ────────────────────────────────────────────────────────────────────────────
// Panel identifiers — these are the ImGui window titles.
// Use "###id" suffix if you ever want to change the displayed title
// without breaking the docking layout persistence.
// ────────────────────────────────────────────────────────────────────────────
static constexpr const char *kPanelViewport   = "3D Viewport###viewport";
static constexpr const char *kPanelNodeEditor = "Node Graph###node_editor";
static constexpr const char *kPanelProperties = "Properties###properties";
static constexpr const char *kPanelLogOutput  = "Output###log_output";
static constexpr const char *kPanelNodeLib    = "Node Library###node_library";

// ────────────────────────────────────────────────────────────────────────────
// build_default_layout()
//
// Called ONCE on first launch (or when the user resets the layout).
// Uses ImGui::DockBuilder to programmatically split the dockspace
// into the default arrangement:
//
//  ┌──────────────────────────────────────────┬──────────────┐
//  │                                          │              │
//  │           3D Viewport                    │  Properties  │
//  │                                          │  Inspector   │
//  │                                          │              │
//  ├──────────────────────────────────────────┤              │
//  │                                          │              │
//  │           Node Graph Editor              │              │
//  │                                          │              │
//  │                                          ├──────────────┤
//  ├──────────────────────────────────────────┤  Node Lib    │
//  │  Output / Log                            │              │
//  └──────────────────────────────────────────┴──────────────┘
//
// ────────────────────────────────────────────────────────────────────────────
inline void build_default_layout(ImGuiID dockspace_id)
{
    // Clear any existing layout for this dockspace
    ImGui::DockBuilderRemoveNode(dockspace_id);

    // Create the root node — fill the entire dockspace area
    ImGui::DockBuilderAddNode(dockspace_id,
                              ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id,
                                  ImGui::GetMainViewport()->Size);

    // ── Step 1: Split off the right panel (Properties) ──────────────
    //    ~22% of width goes to the right
    ImGuiID dock_right = 0;
    ImGuiID dock_main  = 0;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.22f,
                                &dock_right, &dock_main);

    // ── Step 2: Split right panel vertically ────────────────────────
    //    Top 70% = Properties, Bottom 30% = Node Library
    ImGuiID dock_right_bottom = 0;
    ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.30f,
                                &dock_right_bottom, &dock_right);

    // ── Step 3: Split the main area vertically ──────────────────────
    //    Top 50% = 3D Viewport, Bottom 50% = Node Graph
    ImGuiID dock_top    = 0;
    ImGuiID dock_bottom = 0;
    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.55f,
                                &dock_bottom, &dock_top);

    // ── Step 4: Split a thin log panel from the bottom of nodes ─────
    ImGuiID dock_log = 0;
    ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Down, 0.18f,
                                &dock_log, &dock_bottom);

    // ── Step 5: Assign windows to dock slots ────────────────────────
    ImGui::DockBuilderDockWindow(kPanelViewport,   dock_top);
    ImGui::DockBuilderDockWindow(kPanelNodeEditor,  dock_bottom);
    ImGui::DockBuilderDockWindow(kPanelProperties,  dock_right);
    ImGui::DockBuilderDockWindow(kPanelNodeLib,     dock_right_bottom);
    ImGui::DockBuilderDockWindow(kPanelLogOutput,   dock_log);

    // Commit the layout
    ImGui::DockBuilderFinish(dockspace_id);
}

// ────────────────────────────────────────────────────────────────────────────
// render_dockspace()
//
// Called EVERY FRAME from your main render loop. This creates the
// full-window dockspace and the top-level menu bar.
//
// Usage:
//     while (running) {
//         ImGui_ImplXXX_NewFrame();
//         ImGui::NewFrame();
//         terralith::ui::render_dockspace();
//         terralith::ui::render_viewport(...);
//         terralith::ui::render_node_editor(...);
//         terralith::ui::render_properties(...);
//         terralith::ui::render_log_output(...);
//         ImGui::Render();
//     }
// ────────────────────────────────────────────────────────────────────────────
inline void render_dockspace()
{
    // ── Fullscreen host window ──────────────────────────────────────
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar       |
        ImGuiWindowFlags_NoCollapse       |
        ImGuiWindowFlags_NoResize         |
        ImGuiWindowFlags_NoMove           |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus       |
        ImGuiWindowFlags_NoDocking        |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("TerraLith##DockHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    // ── Main Menu Bar ───────────────────────────────────────────────
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project",     "Ctrl+N"))  { /* ... */ }
            if (ImGui::MenuItem("Open Project",    "Ctrl+O"))  { /* ... */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Save",            "Ctrl+S"))  { /* ... */ }
            if (ImGui::MenuItem("Save As...",      "Ctrl+Shift+S")) { /* ... */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Heightmap"))            { /* ... */ }
            if (ImGui::MenuItem("Bake All Nodes"))             { /* ... */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit",            "Alt+F4"))  { /* ... */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo",            "Ctrl+Z"))  { /* ... */ }
            if (ImGui::MenuItem("Redo",            "Ctrl+Y"))  { /* ... */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Project Settings"))           { /* ... */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem(kPanelViewport,   nullptr, nullptr);
            ImGui::MenuItem(kPanelNodeEditor,  nullptr, nullptr);
            ImGui::MenuItem(kPanelProperties,  nullptr, nullptr);
            ImGui::MenuItem(kPanelNodeLib,     nullptr, nullptr);
            ImGui::MenuItem(kPanelLogOutput,   nullptr, nullptr);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout"))
            {
                // Force rebuild on next frame
                ImGuiID id = ImGui::GetID("TerraLithDockSpace");
                build_default_layout(id);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Quick Start"))    { /* ... */ }
            if (ImGui::MenuItem("Documentation"))  { /* ... */ }
            ImGui::Separator();
            if (ImGui::MenuItem("About Hesiod"))   { /* ... */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // ── DockSpace ───────────────────────────────────────────────────
    ImGuiID dockspace_id = ImGui::GetID("TerraLithDockSpace");

    // Build default layout on first run (when no imgui.ini exists)
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
        build_default_layout(dockspace_id);
    }

    ImGuiDockNodeFlags dockspace_flags =
        ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End(); // TerraLith##DockHost
}

// ────────────────────────────────────────────────────────────────────────────
// Panel rendering stubs — each becomes a dockable window.
// Fill these with your actual rendering logic.
// ────────────────────────────────────────────────────────────────────────────

// 3D Viewport: renders the terrain preview to an FBO, displays via ImTextureID
inline void render_viewport_panel(/* ImTextureID fbo_texture, int w, int h */)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin(kPanelViewport))
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        // Example: render FBO at panel size, display as image
        // resize_fbo(static_cast<int>(avail.x), static_cast<int>(avail.y));
        // render_terrain_to_fbo();
        // ImGui::Image(fbo_texture, avail);

        // Overlay controls (camera mode, render mode toggle)
        ImGui::SetCursorPos(ImVec2(8, 8));
        ImGui::BeginGroup();
        // ImGui::SmallButton("Orbit"); ImGui::SameLine();
        // ImGui::SmallButton("Pan");   ImGui::SameLine();
        // ImGui::SmallButton("Wireframe");
        ImGui::EndGroup();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// Node Graph: hosts your node editor (imnodes / imgui-node-editor)
inline void render_node_editor_panel()
{
    if (ImGui::Begin(kPanelNodeEditor))
    {
        // Your node editor rendering goes here
        // e.g., ImNodes::BeginNodeEditor(); ... ImNodes::EndNodeEditor();
        // or ax::NodeEditor::Begin("NodeEd"); ... ax::NodeEditor::End();
    }
    ImGui::End();
}

// Properties Inspector: shows attributes of the selected node
inline void render_properties_panel()
{
    if (ImGui::Begin(kPanelProperties))
    {
        // Populated when a node is selected
        // See properties_inspector.hpp for detailed implementation
    }
    ImGui::End();
}

// Node Library: searchable catalog of available nodes
inline void render_node_library_panel()
{
    if (ImGui::Begin(kPanelNodeLib))
    {
        static char search_buf[128] = "";
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Search nodes...", search_buf,
                                 sizeof(search_buf));
        ImGui::Separator();
        // Tree of node categories + drag-to-canvas support
    }
    ImGui::End();
}

// Output / Log panel
inline void render_log_panel()
{
    if (ImGui::Begin(kPanelLogOutput))
    {
        // Scrolling log with severity filtering
    }
    ImGui::End();
}

} // namespace terralith::ui
