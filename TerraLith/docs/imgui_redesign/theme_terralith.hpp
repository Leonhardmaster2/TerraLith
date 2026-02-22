#pragma once
// ============================================================================
// TerraLith — Modern Dark Theme for Dear ImGui
// ============================================================================
// A polished, professional dark theme inspired by Substance Designer,
// Blender, and modern creative tools. Avoids pure black (#000000),
// uses warm dark greys with teal/amber accent colors.
//
// Call terralith::ui::apply_theme() once after ImGui context creation.
// ============================================================================

#include "imgui.h"

namespace terralith::ui
{

// ────────────────────────────────────────────────────────────────────────────
// Color palette — centralized so you can swap accent colors in one place.
// All values in linear 0–1 range.
// ────────────────────────────────────────────────────────────────────────────
namespace palette
{
    // ── Base surfaces (warm dark greys, never pure black) ───────────
    constexpr ImVec4 bg_darkest    = {0.098f, 0.098f, 0.110f, 1.00f}; // #19191C
    constexpr ImVec4 bg_dark       = {0.118f, 0.118f, 0.133f, 1.00f}; // #1E1E22
    constexpr ImVec4 bg_mid        = {0.149f, 0.149f, 0.165f, 1.00f}; // #26262A
    constexpr ImVec4 bg_light      = {0.188f, 0.188f, 0.204f, 1.00f}; // #303034
    constexpr ImVec4 bg_lighter    = {0.224f, 0.224f, 0.243f, 1.00f}; // #39393E

    // ── Text ────────────────────────────────────────────────────────
    constexpr ImVec4 text_primary  = {0.878f, 0.886f, 0.910f, 1.00f}; // #E0E2E8
    constexpr ImVec4 text_dim      = {0.502f, 0.514f, 0.553f, 1.00f}; // #80838D
    constexpr ImVec4 text_disabled = {0.353f, 0.361f, 0.392f, 1.00f}; // #5A5C64

    // ── Primary accent (teal / cyan) ────────────────────────────────
    constexpr ImVec4 accent        = {0.263f, 0.588f, 0.698f, 1.00f}; // #4396B2
    constexpr ImVec4 accent_hover  = {0.318f, 0.663f, 0.773f, 1.00f}; // #51A9C5
    constexpr ImVec4 accent_active = {0.208f, 0.502f, 0.608f, 1.00f}; // #35809B
    constexpr ImVec4 accent_dim    = {0.263f, 0.588f, 0.698f, 0.31f}; // 31% opacity

    // ── Secondary accent (warm amber/gold for warnings, highlights) ─
    constexpr ImVec4 warm          = {0.835f, 0.573f, 0.208f, 1.00f}; // #D59235
    constexpr ImVec4 warm_dim      = {0.835f, 0.573f, 0.208f, 0.40f};

    // ── Semantic colors ─────────────────────────────────────────────
    constexpr ImVec4 success       = {0.306f, 0.686f, 0.420f, 1.00f}; // #4EAF6B
    constexpr ImVec4 error         = {0.784f, 0.306f, 0.314f, 1.00f}; // #C84E50
    constexpr ImVec4 warning       = warm;

    // ── Borders and separators ──────────────────────────────────────
    constexpr ImVec4 border        = {0.200f, 0.200f, 0.220f, 1.00f}; // #333338
    constexpr ImVec4 border_light  = {0.255f, 0.255f, 0.278f, 1.00f}; // #414147
    constexpr ImVec4 separator     = {0.200f, 0.200f, 0.220f, 0.60f};

    // ── Scrollbar ───────────────────────────────────────────────────
    constexpr ImVec4 scrollbar_bg  = {0.098f, 0.098f, 0.110f, 0.53f};
    constexpr ImVec4 scrollbar_grab= {0.275f, 0.275f, 0.298f, 1.00f};
    constexpr ImVec4 scrollbar_hov = {0.353f, 0.353f, 0.380f, 1.00f};

    // ── Tab colors ──────────────────────────────────────────────────
    constexpr ImVec4 tab_normal    = {0.118f, 0.118f, 0.133f, 1.00f};
    constexpr ImVec4 tab_hovered   = {0.188f, 0.188f, 0.210f, 1.00f};
    constexpr ImVec4 tab_active    = {0.165f, 0.165f, 0.184f, 1.00f};
    constexpr ImVec4 tab_unfocused = {0.110f, 0.110f, 0.125f, 1.00f};
}

// ────────────────────────────────────────────────────────────────────────────
// apply_theme()
//
// Sets both the ImGuiStyle metrics (rounding, padding, spacing) and
// the full color table. Call once after ImGui::CreateContext().
// ────────────────────────────────────────────────────────────────────────────
inline void apply_theme()
{
    ImGuiStyle &style = ImGui::GetStyle();
    using namespace palette;

    // ================================================================
    //  METRICS — Geometry, spacing, rounding
    // ================================================================

    // Window
    style.WindowPadding      = ImVec2(10.0f, 10.0f);
    style.WindowRounding     = 6.0f;
    style.WindowBorderSize   = 1.0f;
    style.WindowMinSize      = ImVec2(120.0f, 80.0f);
    style.WindowTitleAlign   = ImVec2(0.02f, 0.50f);

    // Frame (buttons, inputs, sliders)
    style.FramePadding       = ImVec2(8.0f, 5.0f);
    style.FrameRounding      = 4.0f;
    style.FrameBorderSize    = 0.0f;

    // Items
    style.ItemSpacing        = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing   = ImVec2(6.0f, 4.0f);
    style.IndentSpacing      = 20.0f;

    // Widgets
    style.CellPadding        = ImVec2(6.0f, 3.0f);
    style.ScrollbarSize      = 12.0f;
    style.ScrollbarRounding  = 8.0f;
    style.GrabMinSize        = 8.0f;
    style.GrabRounding       = 3.0f;

    // Tabs
    style.TabRounding        = 4.0f;
    style.TabBorderSize      = 0.0f;
    style.TabBarBorderSize   = 1.0f;
    style.TabBarOverlineSize = 2.0f; // colored top-line on active tab

    // Popups
    style.PopupRounding      = 6.0f;
    style.PopupBorderSize    = 1.0f;

    // Separators
    style.SeparatorTextBorderSize = 2.0f;

    // Docking
    style.DockingSeparatorSize = 2.0f;

    // Anti-aliasing (leave on for smooth rounded corners)
    style.AntiAliasedLines   = true;
    style.AntiAliasedFill    = true;

    // ================================================================
    //  COLORS
    // ================================================================
    ImVec4 *c = style.Colors;

    // ── Text ────────────────────────────────────────────────────────
    c[ImGuiCol_Text]                  = text_primary;
    c[ImGuiCol_TextDisabled]          = text_disabled;

    // ── Backgrounds ─────────────────────────────────────────────────
    c[ImGuiCol_WindowBg]              = bg_dark;
    c[ImGuiCol_ChildBg]               = {0, 0, 0, 0};          // transparent
    c[ImGuiCol_PopupBg]               = {bg_mid.x, bg_mid.y, bg_mid.z, 0.96f};

    // ── Borders ─────────────────────────────────────────────────────
    c[ImGuiCol_Border]                = border;
    c[ImGuiCol_BorderShadow]          = {0, 0, 0, 0};

    // ── Frame (inputs, checkboxes, sliders) ─────────────────────────
    c[ImGuiCol_FrameBg]               = bg_mid;
    c[ImGuiCol_FrameBgHovered]        = bg_light;
    c[ImGuiCol_FrameBgActive]         = bg_lighter;

    // ── Title Bar ───────────────────────────────────────────────────
    c[ImGuiCol_TitleBg]               = bg_darkest;
    c[ImGuiCol_TitleBgActive]         = bg_dark;
    c[ImGuiCol_TitleBgCollapsed]      = {bg_darkest.x, bg_darkest.y, bg_darkest.z, 0.75f};

    // ── Menu Bar ────────────────────────────────────────────────────
    c[ImGuiCol_MenuBarBg]             = bg_darkest;

    // ── Scrollbar ───────────────────────────────────────────────────
    c[ImGuiCol_ScrollbarBg]           = scrollbar_bg;
    c[ImGuiCol_ScrollbarGrab]         = scrollbar_grab;
    c[ImGuiCol_ScrollbarGrabHovered]  = scrollbar_hov;
    c[ImGuiCol_ScrollbarGrabActive]   = accent;

    // ── Checkmark / Slider Grab ─────────────────────────────────────
    c[ImGuiCol_CheckMark]             = accent;
    c[ImGuiCol_SliderGrab]            = accent;
    c[ImGuiCol_SliderGrabActive]      = accent_active;

    // ── Buttons ─────────────────────────────────────────────────────
    c[ImGuiCol_Button]                = bg_light;
    c[ImGuiCol_ButtonHovered]         = accent_dim;
    c[ImGuiCol_ButtonActive]          = accent_active;

    // ── Headers (collapsing headers, tree nodes, selectable) ────────
    c[ImGuiCol_Header]                = bg_light;
    c[ImGuiCol_HeaderHovered]         = accent_dim;
    c[ImGuiCol_HeaderActive]          = accent;

    // ── Separator ───────────────────────────────────────────────────
    c[ImGuiCol_Separator]             = separator;
    c[ImGuiCol_SeparatorHovered]      = accent;
    c[ImGuiCol_SeparatorActive]       = accent_active;

    // ── Resize Grip ─────────────────────────────────────────────────
    c[ImGuiCol_ResizeGrip]            = {accent.x, accent.y, accent.z, 0.15f};
    c[ImGuiCol_ResizeGripHovered]     = {accent.x, accent.y, accent.z, 0.50f};
    c[ImGuiCol_ResizeGripActive]      = accent;

    // ── Tabs ────────────────────────────────────────────────────────
    c[ImGuiCol_Tab]                   = tab_normal;
    c[ImGuiCol_TabHovered]            = tab_hovered;
    c[ImGuiCol_TabSelected]           = tab_active;
    c[ImGuiCol_TabSelectedOverline]   = accent;       // teal top-line
    c[ImGuiCol_TabDimmed]             = tab_unfocused;
    c[ImGuiCol_TabDimmedSelected]     = {tab_active.x, tab_active.y, tab_active.z, 0.80f};
    c[ImGuiCol_TabDimmedSelectedOverline] = {accent.x, accent.y, accent.z, 0.40f};

    // ── Docking ─────────────────────────────────────────────────────
    c[ImGuiCol_DockingPreview]        = {accent.x, accent.y, accent.z, 0.60f};
    c[ImGuiCol_DockingEmptyBg]        = bg_darkest;

    // ── Tables ──────────────────────────────────────────────────────
    c[ImGuiCol_TableHeaderBg]         = bg_mid;
    c[ImGuiCol_TableBorderStrong]     = border;
    c[ImGuiCol_TableBorderLight]      = {border.x, border.y, border.z, 0.50f};
    c[ImGuiCol_TableRowBg]            = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt]         = {1.0f, 1.0f, 1.0f, 0.015f};

    // ── Drag/Drop ───────────────────────────────────────────────────
    c[ImGuiCol_DragDropTarget]        = warm;

    // ── Nav ─────────────────────────────────────────────────────────
    c[ImGuiCol_NavCursor]             = accent;
    c[ImGuiCol_NavWindowingHighlight] = {accent.x, accent.y, accent.z, 0.70f};
    c[ImGuiCol_NavWindowingDimBg]     = {0.10f, 0.10f, 0.10f, 0.50f};

    // ── Modal dim ───────────────────────────────────────────────────
    c[ImGuiCol_ModalWindowDimBg]      = {0.0f, 0.0f, 0.0f, 0.60f};

    // ── Text selection ──────────────────────────────────────────────
    c[ImGuiCol_TextSelectedBg]        = {accent.x, accent.y, accent.z, 0.30f};

    // ── Input text cursor ───────────────────────────────────────────
    c[ImGuiCol_TextLink]              = accent_hover;
}

// ────────────────────────────────────────────────────────────────────────────
// Utility: push accent-colored button style (for primary action buttons)
// ────────────────────────────────────────────────────────────────────────────
inline void push_accent_button_style()
{
    using namespace palette;
    ImGui::PushStyleColor(ImGuiCol_Button,        accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  accent_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   accent_active);
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1, 1, 1, 1));
}

inline void pop_accent_button_style()
{
    ImGui::PopStyleColor(4);
}

// ────────────────────────────────────────────────────────────────────────────
// Utility: push warning-colored button style (for destructive actions)
// ────────────────────────────────────────────────────────────────────────────
inline void push_warning_button_style()
{
    using namespace palette;
    ImGui::PushStyleColor(ImGuiCol_Button,        error);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(error.x + 0.08f, error.y + 0.05f,
                                 error.z + 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(error.x - 0.05f, error.y - 0.05f,
                                 error.z - 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1, 1, 1, 1));
}

inline void pop_warning_button_style()
{
    ImGui::PopStyleColor(4);
}

// ────────────────────────────────────────────────────────────────────────────
// Node-type accent colors — for node headers and link wires
// ────────────────────────────────────────────────────────────────────────────
namespace node_colors
{
    constexpr ImU32 heightmap   = IM_COL32( 67, 150, 178, 255); // Teal
    constexpr ImU32 mask        = IM_COL32(213, 146,  53, 255); // Amber
    constexpr ImU32 texture     = IM_COL32(126, 178,  67, 255); // Green
    constexpr ImU32 geometry    = IM_COL32(178,  96, 178, 255); // Purple
    constexpr ImU32 math        = IM_COL32(178, 178,  96, 255); // Yellow
    constexpr ImU32 io_node     = IM_COL32(178,  67,  80, 255); // Red
    constexpr ImU32 routing     = IM_COL32(120, 120, 140, 255); // Grey
    constexpr ImU32 erosion     = IM_COL32( 80, 160, 200, 255); // Light blue
    constexpr ImU32 filter      = IM_COL32(200, 120,  80, 255); // Coral
}

} // namespace terralith::ui
