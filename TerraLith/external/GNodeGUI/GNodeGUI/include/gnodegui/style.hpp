/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */

/**
 * @file style.hpp
 * @author Otto Link (otto.link.bv@gmail.com)
 * @brief
 * @copyright Copyright (c) 2024 Otto Link. Distributed under the terms of the
 * GNU General Public License. See the file LICENSE for the full license.
 */
#pragma once
#include <map>

#include <QColor>
#include <QPoint>

#define GN_STYLE gngui::Style::get_style()

namespace gngui
{

class Style
{
public:
  Style() = default;

  static std::shared_ptr<Style> &get_style();

  struct Viewer
  {
    QColor color_bg = QColor(42, 42, 42, 255);
    QColor color_toolbar = Qt::lightGray;

    bool   add_toolbar = true;
    QPoint toolbar_window_pos = QPoint(10, 40);
    float  toolbar_width = 32.f;
    bool   add_viewport_icon = true;
    bool   add_new_icon = true;
    bool   add_import_icon = true;
    bool   add_load_save_icons = true;
    bool   add_group = true;

    bool disable_during_update = true;
  } viewer;

  struct Node
  {
    float width = 220.f;
    float padding = 10.f;
    float padding_widget_width = 8.f;
    float padding_widget_height = 8.f;
    float rounding_radius = 8.f;
    float port_radius = 7.5f;
    float port_radius_not_selectable = 5.f;
    float port_hit_radius_scale = 2.5f;   // hitbox is this * port_radius (invisible)
    float port_hover_visual_scale = 1.3f; // visual scale-up on hover
    float port_glow_alpha = 80.f;         // 0-255, semi-transparent glow ring
    float port_stroke_width = 2.f;        // hollow port outline stroke width
    float vertical_stretching = 1.3f;
    float header_height_scale = 2.2f;     // taller header to fit title + category

    bool reload_button = true;
    bool settings_button = true;

    // TerraLith dark theme node colors
    QColor color_bg = QColor(30, 30, 34, 255);            // #1E1E22 node body
    QColor color_bg_light = QColor(42, 42, 48, 255);      // #2A2A30 header base
    QColor color_border = QColor(51, 51, 56, 255);        // #333338 subtle border
    QColor color_border_hovered = QColor(74, 74, 82, 255); // #4A4A52
    QColor color_caption = QColor(224, 226, 232, 255);     // #E0E2E8 title text
    QColor color_caption_dim = QColor(128, 131, 141, 255); // #80838D dim subtitle
    QColor color_selected = QColor(80, 250, 123, 255);
    QColor color_pinned = QColor(139, 233, 253, 255);
    QColor color_icon = QColor(160, 160, 170, 255);
    QColor color_comment = QColor(255, 121, 198, 255);

    QColor color_port_hovered = Qt::white;
    QColor color_port_selected = QColor(80, 250, 123, 255);

    float pen_width = 1.5;
    float pen_width_hovered = 2;
    float pen_width_selected = 2;

    QColor color_port_data_default = Qt::lightGray;
    QColor color_port_not_selectable = QColor(60, 60, 65, 255); // dim on dark bg

    std::map<std::string, QColor> color_port_data = {};
    std::map<std::string, QColor> color_category = {};
  } node;

  struct Link
  {
    float  pen_width = 2.5f;
    float  pen_width_hovered = 3.5f;
    float  pen_width_selected = 4.f;
    float  port_tip_radius = 3.5f;
    float  curvature = 0.5f;
    QColor color_default = QColor(180, 180, 190, 255);
    QColor color_selected = QColor(80, 250, 123, 255);
  } link;

  struct Group
  {
    float default_width = 256.f;
    float default_height = 256.f;
    float pen_width = 1.f;
    float pen_width_hovered = 1.f;
    float pen_width_selected = 3.f;
    float rounding_radius = 16.f;

    QColor color = Qt::white;
    float  background_fill_alpha = 0.1f;
    QColor color_selected = QColor(80, 250, 123, 255);

    bool bold_caption = true;

    std::map<std::string, QColor> color_map = {{"White", Qt::white},
                                               {"Cyan", QColor(139, 233, 253)},
                                               {"Green", QColor(80, 250, 123)},
                                               {"Orange", QColor(255, 184, 108)},
                                               {"Pink", QColor(255, 121, 198)},
                                               {"Purple", QColor(189, 147, 249)},
                                               {"Red", QColor(255, 85, 85)},
                                               {"Yellow", QColor(241, 250, 140)},
                                               {"Black", Qt::black}};
  } group;

  struct Comment
  {
    float rounding_radius = 4.f;
    float width = 256.f;

    QColor color_text = Qt::lightGray;
    QColor color_bg = QColor(108, 108, 108, 255);
    float  background_fill_alpha = 0.1f;
  } comment;

private:
  // Disable copy constructor
  Style(const Style &) = delete;

  // Disable assignment operator
  Style &operator=(const Style &) = delete;

  // Static member to hold the singleton instance
  static std::shared_ptr<Style> instance;
};

QColor get_color_from_data_type(const std::string &data_type);

} // namespace gngui
