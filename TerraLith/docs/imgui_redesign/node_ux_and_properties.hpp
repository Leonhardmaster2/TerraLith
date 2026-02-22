#pragma once
// ============================================================================
// TerraLith — Node UX Improvements & Properties Inspector Patterns
// ============================================================================
// This file demonstrates:
//   1. Custom node rendering with colored headers, thick bezier links,
//      and improved pin visuals (using imgui-node-editor / ax::NodeEditor)
//   2. A clean Properties Inspector panel using ImGui tables
// ============================================================================

#include "imgui.h"
#include "imgui_internal.h"

// If using thedmd/imgui-node-editor:
// #include "imgui-node-editor/imgui_node_editor.h"
// namespace ne = ax::NodeEditor;

// If using Nelarius/imnodes:
// #include "imnodes.h"

#include <string>
#include <vector>

namespace terralith::ui
{

// ════════════════════════════════════════════════════════════════════════════
// PART 1: NODE UX — Colored Headers, Thick Links, Better Pins
// ════════════════════════════════════════════════════════════════════════════

// ────────────────────────────────────────────────────────────────────────────
// Pin data types — each gets a unique color for instant visual parsing
// ────────────────────────────────────────────────────────────────────────────
enum class PinDataType
{
    Heightmap,     // float grid
    Mask,          // single-channel mask
    Texture,       // RGBA color data
    Geometry,      // mesh data
    Scalar,        // single float
    Path,          // 2D path data
    Cloud,         // point cloud
};

inline ImU32 pin_color_for_type(PinDataType type)
{
    switch (type)
    {
        case PinDataType::Heightmap: return IM_COL32( 67, 150, 178, 255); // Teal
        case PinDataType::Mask:      return IM_COL32(213, 146,  53, 255); // Amber
        case PinDataType::Texture:   return IM_COL32(126, 178,  67, 255); // Green
        case PinDataType::Geometry:  return IM_COL32(178,  96, 178, 255); // Purple
        case PinDataType::Scalar:    return IM_COL32(200, 200, 210, 255); // Light grey
        case PinDataType::Path:      return IM_COL32(200, 120,  80, 255); // Coral
        case PinDataType::Cloud:     return IM_COL32( 80, 160, 200, 255); // Sky blue
        default:                     return IM_COL32(150, 150, 150, 255);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// draw_pin()
//
// Renders a filled circle pin with a brighter ring when hovered.
// Much larger and more visible than default imnodes pins.
//
// Usage within a node body (imnodes example):
//     ImNodes::BeginInputAttribute(pin_id);
//     draw_pin(PinDataType::Heightmap, true, is_connected);
//     ImGui::SameLine();
//     ImGui::TextUnformatted("Height In");
//     ImNodes::EndInputAttribute();
// ────────────────────────────────────────────────────────────────────────────
inline void draw_pin(PinDataType type, bool is_input, bool is_connected)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 cursor  = ImGui::GetCursorScreenPos();

    constexpr float radius       = 6.0f;  // much bigger than default ~3px
    constexpr float hover_ring   = 9.0f;
    constexpr int   segments     = 16;

    ImVec2 center = {cursor.x + radius, cursor.y + radius + 2.0f};
    ImU32 color   = pin_color_for_type(type);
    ImU32 fill    = is_connected ? color : IM_COL32(40, 40, 45, 255);

    // Invisible button for hit detection (larger than visual)
    ImGui::InvisibleButton("##pin", ImVec2(radius * 2.0f + 4.0f,
                                           radius * 2.0f + 4.0f));
    bool hovered = ImGui::IsItemHovered();

    // Hover glow ring
    if (hovered)
    {
        dl->AddCircleFilled(center, hover_ring,
                            IM_COL32(ImU8((color >> 0) & 0xFF),
                                     ImU8((color >> 8) & 0xFF),
                                     ImU8((color >> 16) & 0xFF), 50),
                            segments);
    }

    // Outer ring (always visible, provides contrast)
    dl->AddCircle(center, radius, color, segments, 2.0f);

    // Fill (only when connected)
    if (is_connected)
        dl->AddCircleFilled(center, radius - 1.5f, fill, segments);
    else
        dl->AddCircleFilled(center, radius - 1.5f, fill, segments);
}

// ────────────────────────────────────────────────────────────────────────────
// draw_node_header()
//
// Custom-drawn colored header bar at the top of each node.
// Call this AFTER beginning the node (so you have the node's screen rect).
//
// For imgui-node-editor (ax::NodeEditor), you'd use:
//     ne::BeginNode(nodeId);
//     draw_node_header("Perlin Noise", node_colors::heightmap);
//     // ... pins and body ...
//     ne::EndNode();
//
// For imnodes, you'd use the built-in title bar coloring:
//     ImNodes::PushColorStyle(ImNodesCol_TitleBar, color);
//     ImNodes::BeginNode(id);
//     ImNodes::BeginNodeTitleBar();
//     ImGui::TextUnformatted(title);
//     ImNodes::EndNodeTitleBar();
//     ImNodes::EndNode();
//     ImNodes::PopColorStyle();
// ────────────────────────────────────────────────────────────────────────────
inline void draw_node_header(const char *title, ImU32 header_color,
                             float node_width = 180.0f)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 pos     = ImGui::GetCursorScreenPos();

    constexpr float header_h      = 28.0f;
    constexpr float corner_radius = 6.0f;

    ImVec2 header_min = pos;
    ImVec2 header_max = {pos.x + node_width, pos.y + header_h};

    // Colored header with top-rounded corners only
    dl->AddRectFilled(header_min, header_max, header_color,
                      corner_radius, ImDrawFlags_RoundCornersTop);

    // Subtle gradient fade at bottom of header for polish
    ImU32 fade = IM_COL32(0, 0, 0, 40);
    dl->AddRectFilledMultiColor(
        {header_min.x, header_max.y - 6.0f}, header_max,
        IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), fade, fade);

    // Title text (centered vertically)
    ImVec2 text_size = ImGui::CalcTextSize(title);
    ImVec2 text_pos  = {header_min.x + 10.0f,
                        header_min.y + (header_h - text_size.y) * 0.5f};
    dl->AddText(text_pos, IM_COL32(255, 255, 255, 230), title);

    // Advance cursor past the header
    ImGui::Dummy(ImVec2(node_width, header_h + 4.0f));
}

// ────────────────────────────────────────────────────────────────────────────
// configure_link_style()
//
// Sets up thick, smooth bezier curves for node connections.
// Call once during initialization.
//
// For imnodes:
//     ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 3.0f);
//     ImNodes::PushStyleVar(ImNodesStyleVar_LinkLineSegmentsPerLength, 0.1f);
//
// For imgui-node-editor (ne::Style):
//     auto &edStyle = ne::GetStyle();
//     edStyle.FlowMarkerDistance  = 30.0f;
//     edStyle.FlowSpeed          = 150.0f;
//     edStyle.FlowDuration       = 2.0f;
//     edStyle.LinkStrength        = 200.0f;   // tighter cubic curves
//     edStyle.PinRounding         = 6.0f;
//     edStyle.PivotSize           = ImVec2(0, 0); // no midpoint diamond
// ────────────────────────────────────────────────────────────────────────────

// Example for custom bezier drawing (if doing it manually):
inline void draw_thick_bezier_link(ImDrawList *dl,
                                   ImVec2 p0, ImVec2 p3,
                                   ImU32  color,
                                   float  thickness = 3.0f,
                                   bool   is_hovered = false,
                                   bool   animated_flow = false)
{
    // Compute control points for a natural horizontal bezier
    float dist = ImAbs(p3.x - p0.x) * 0.5f;
    dist = ImMax(dist, 50.0f); // minimum curvature
    ImVec2 p1 = {p0.x + dist, p0.y};
    ImVec2 p2 = {p3.x - dist, p3.y};

    float draw_thickness = is_hovered ? thickness + 1.5f : thickness;

    // Shadow/glow behind the link for depth
    if (is_hovered)
    {
        ImU32 glow = IM_COL32(ImU8((color >> 0) & 0xFF),
                              ImU8((color >> 8) & 0xFF),
                              ImU8((color >> 16) & 0xFF), 40);
        dl->AddBezierCubic(p0, p1, p2, p3, glow, draw_thickness + 4.0f);
    }

    // Main link
    dl->AddBezierCubic(p0, p1, p2, p3, color, draw_thickness);

    // Animated flow dots (optional — shows data direction)
    if (animated_flow)
    {
        float t = ImFmod((float)ImGui::GetTime() * 0.8f, 1.0f);
        for (int i = 0; i < 3; ++i)
        {
            float ti = ImFmod(t + i * 0.33f, 1.0f);
            ImVec2 dot;
            // De Casteljau evaluation
            ImVec2 a = ImLerp(p0, p1, ti);
            ImVec2 b = ImLerp(p1, p2, ti);
            ImVec2 c = ImLerp(p2, p3, ti);
            ImVec2 d = ImLerp(a, b, ti);
            ImVec2 e = ImLerp(b, c, ti);
            dot = ImLerp(d, e, ti);
            dl->AddCircleFilled(dot, 3.0f, IM_COL32(255, 255, 255, 180));
        }
    }
}


// ════════════════════════════════════════════════════════════════════════════
// PART 2: PROPERTIES INSPECTOR — Clean, Aligned, Professional
// ════════════════════════════════════════════════════════════════════════════

// ────────────────────────────────────────────────────────────────────────────
// Helper: Section header with separator
// ────────────────────────────────────────────────────────────────────────────
inline bool collapsing_section(const char *label, bool default_open = true)
{
    ImGui::PushStyleColor(ImGuiCol_Header,
                          ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                          ImVec4(0.22f, 0.22f, 0.25f, 1.0f));

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed
                             | ImGuiTreeNodeFlags_AllowOverlap;
    if (default_open) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool open = ImGui::TreeNodeEx(label, flags);
    ImGui::PopStyleColor(2);
    return open;
}

// ────────────────────────────────────────────────────────────────────────────
// Helper: Table-based property row
//
// Creates a two-column layout:
//   | Label (dimmed, right-aligned) | Widget (fills remaining width) |
//
// Usage:
//   if (begin_property("Amplitude")) {
//       ImGui::SliderFloat("##amp", &amplitude, 0.0f, 1.0f);
//       end_property();
//   }
// ────────────────────────────────────────────────────────────────────────────
inline bool begin_property(const char *label, float label_width_fraction = 0.38f)
{
    bool visible = ImGui::BeginTable("##prop", 2, ImGuiTableFlags_None);
    if (visible)
    {
        ImGui::TableSetupColumn("label",
            ImGuiTableColumnFlags_WidthFixed,
            ImGui::GetContentRegionAvail().x * label_width_fraction);
        ImGui::TableSetupColumn("widget",
            ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Right-aligned, dimmed label
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0.55f, 0.56f, 0.60f, 1.0f));
        float text_w = ImGui::CalcTextSize(label).x;
        float col_w  = ImGui::GetContentRegionAvail().x;
        if (text_w < col_w)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + col_w - text_w - 4.0f);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1); // widget fills the column
    }
    return visible;
}

inline void end_property()
{
    ImGui::PopItemWidth();
    ImGui::EndTable();
}

// ────────────────────────────────────────────────────────────────────────────
// Helper: Compact Vec3 editor (X/Y/Z on one row, color-coded)
// ────────────────────────────────────────────────────────────────────────────
inline bool vec3_editor(const char *label, float v[3],
                        float v_min = 0.0f, float v_max = 1.0f)
{
    bool changed = false;
    if (begin_property(label))
    {
        float w = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;

        // X — Red accent
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.30f, 0.12f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.38f, 0.15f, 0.15f, 1.0f));
        ImGui::PushItemWidth(w);
        changed |= ImGui::DragFloat("##x", &v[0], 0.01f, v_min, v_max, "X: %.2f");
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 4);

        // Y — Green accent
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.28f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.35f, 0.15f, 1.0f));
        ImGui::PushItemWidth(w);
        changed |= ImGui::DragFloat("##y", &v[1], 0.01f, v_min, v_max, "Y: %.2f");
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 4);

        // Z — Blue accent
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.15f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.18f, 0.38f, 1.0f));
        ImGui::PushItemWidth(w);
        changed |= ImGui::DragFloat("##z", &v[2], 0.01f, v_min, v_max, "Z: %.2f");
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2);

        end_property();
    }
    return changed;
}

// ────────────────────────────────────────────────────────────────────────────
// Example: Full properties panel for a "Perlin Noise" node
//
// Demonstrates the patterns in context: sections, property rows,
// styled sliders, dropdowns, and the Vec3 editor.
// ────────────────────────────────────────────────────────────────────────────
inline void example_perlin_noise_properties()
{
    // ── Node title bar at top of properties ──────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              ImVec4(0.263f, 0.588f, 0.698f, 0.15f));
        ImGui::BeginChild("##node_header", ImVec2(-1, 42), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPos(ImVec2(12, 6));

        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0.263f, 0.588f, 0.698f, 1.0f));
        ImGui::Text("HEIGHTMAP");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0.88f, 0.89f, 0.91f, 1.0f));
        ImGui::Text("Perlin Noise");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(12, 24));
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0.50f, 0.51f, 0.55f, 1.0f));
        ImGui::TextUnformatted("Generates coherent noise heightmap");
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // ── Parameters section ──────────────────────────────────────────
    if (collapsing_section("Parameters"))
    {
        static float frequency   = 4.0f;
        static float amplitude   = 1.0f;
        static int   octaves     = 6;
        static float persistence = 0.5f;
        static float lacunarity  = 2.0f;
        static int   seed        = 42;
        static int   noise_type  = 0;

        if (begin_property("Noise Type"))
        {
            const char *types[] = {"Perlin", "Simplex", "Value", "Worley"};
            ImGui::Combo("##ntype", &noise_type, types, IM_ARRAYSIZE(types));
            end_property();
        }

        if (begin_property("Seed"))
        {
            ImGui::DragInt("##seed", &seed, 1.0f, 0, 99999);
            end_property();
        }

        if (begin_property("Frequency"))
        {
            ImGui::SliderFloat("##freq", &frequency, 0.1f, 32.0f, "%.1f");
            end_property();
        }

        if (begin_property("Amplitude"))
        {
            ImGui::SliderFloat("##amp", &amplitude, 0.0f, 2.0f, "%.3f");
            end_property();
        }

        if (begin_property("Octaves"))
        {
            ImGui::SliderInt("##oct", &octaves, 1, 12);
            end_property();
        }

        if (begin_property("Persistence"))
        {
            ImGui::SliderFloat("##pers", &persistence, 0.0f, 1.0f, "%.3f");
            end_property();
        }

        if (begin_property("Lacunarity"))
        {
            ImGui::SliderFloat("##lac", &lacunarity, 1.0f, 4.0f, "%.2f");
            end_property();
        }

        ImGui::TreePop();
    }

    // ── Transform section ───────────────────────────────────────────
    if (collapsing_section("Transform"))
    {
        static float offset[3] = {0.0f, 0.0f, 0.0f};
        static float scale[3]  = {1.0f, 1.0f, 1.0f};
        static float angle     = 0.0f;

        vec3_editor("Offset", offset, -10.0f, 10.0f);
        vec3_editor("Scale",  scale,    0.01f,  5.0f);

        if (begin_property("Rotation"))
        {
            ImGui::SliderFloat("##rot", &angle, 0.0f, 360.0f, "%.1f deg");
            end_property();
        }

        ImGui::TreePop();
    }

    // ── Output section ──────────────────────────────────────────────
    if (collapsing_section("Output", false))
    {
        if (begin_property("Resolution"))
        {
            static int res = 1;
            const char *resolutions[] = {"256", "512", "1024", "2048", "4096"};
            ImGui::Combo("##res", &res, resolutions, IM_ARRAYSIZE(resolutions));
            end_property();
        }

        if (begin_property("Normalize"))
        {
            static bool norm = true;
            ImGui::Checkbox("##norm", &norm);
            end_property();
        }

        ImGui::TreePop();
    }
}

} // namespace terralith::ui
