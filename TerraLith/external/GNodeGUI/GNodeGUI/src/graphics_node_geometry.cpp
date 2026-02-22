/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <QFontMetrics>

#include "gnodegui/graphics_node_geometry.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"

namespace gngui
{

GraphicsNodeGeometry::GraphicsNodeGeometry(NodeProxy *p_node_proxy, QSizeF widget_size)
    : p_node_proxy(p_node_proxy)
{
  if (!p_node_proxy)
  {
    Logger::log()->error(
        "GraphicsNodeGeometry::GraphicsNodeGeometry: p_node_proxy is nullptr");
    return;
  }

  // base increment
  QFont        font;
  QFontMetrics fm(font);

  // in this order...
  this->compute_base_metrics(fm);
  this->compute_node_width(widget_size);
  this->compute_caption(fm);
  this->compute_comment_height(fm, this->p_node_proxy->get_comment());
  this->compute_full_dimensions(widget_size);
  this->compute_body_and_header();
  this->compute_ports(fm);
  this->compute_widget_position();
}

//

void GraphicsNodeGeometry::compute_base_metrics(QFontMetrics &fm)
{
  this->line_height = GN_STYLE->node.vertical_stretching * fm.height();
  this->margin = 2.f * GN_STYLE->node.port_radius;
  this->header_gap = GN_STYLE->node.header_height_scale * this->line_height;
}

void GraphicsNodeGeometry::compute_body_and_header()
{
  // Body starts near the top — the header is inside the body, encompassing
  // the title and category text (no more floating caption above the body).
  float body_top = GN_STYLE->node.padding;
  this->body_rect = QRectF(this->margin,
                           body_top,
                           this->node_width,
                           this->full_height - body_top - this->comment_height);

  this->header_rect = this->body_rect;
  this->header_rect.setHeight(this->header_gap);

  this->comment_rect = QRectF(this->body_rect.bottomLeft(),
                              QSizeF(this->node_width, this->comment_height));
}

void GraphicsNodeGeometry::compute_caption(const QFontMetrics &fm)
{
  // Compute caption size with bold font (matching paint)
  QFont bold_font;
  bold_font.setBold(true);
  QFontMetrics fm_bold(bold_font);

  this->caption_size = fm_bold.size(Qt::TextSingleLine,
                                    this->p_node_proxy->get_caption().c_str());

  // Position title inside the header region with generous padding
  float body_top = GN_STYLE->node.padding;
  this->caption_pos = QPointF(this->margin + 2.f * GN_STYLE->node.padding,
                              body_top + this->line_height);

  // Category subtitle sits below the title
  this->category_pos = QPointF(this->margin + 2.f * GN_STYLE->node.padding,
                               this->caption_pos.y() + 0.85f * this->line_height);
}

void GraphicsNodeGeometry::compute_comment_height(const QFontMetrics &fm,
                                                  const std::string  &comment)
{
  // compute wrapped comment text height and store it
  if (comment.empty())
  {
    this->comment_height = 0.f;
    return;
  }

  const int max_width = int(this->node_width - 2.f * GN_STYLE->node.padding);

  QRect rect = fm.boundingRect(0,
                               0,
                               max_width,
                               10000,
                               Qt::TextWordWrap,
                               QString::fromStdString(comment));

  this->comment_height = rect.height();
}

void GraphicsNodeGeometry::compute_full_dimensions(const QSizeF &widget_size)
{
  float min_width_caption = this->caption_size.width() + 4.f * GN_STYLE->node.padding;
  this->full_width = std::max(min_width_caption, this->node_width) + 2.f * this->margin;

  // Extra bottom padding for the execution time text area
  float bottom_info_area = this->line_height;

  this->full_height = this->line_height * (0.5f + this->p_node_proxy->get_nports()) +
                      this->header_gap + this->comment_height + 2.f * this->margin +
                      bottom_info_area + 2.f * GN_STYLE->node.padding;

  if (widget_size.height() > 0)
  {
    this->full_height += widget_size.height() +
                         2.f * GN_STYLE->node.padding_widget_height;
  }
}

void GraphicsNodeGeometry::compute_node_width(const QSizeF &widget_size)
{
  float min_from_widget = widget_size.width() + 2.f * GN_STYLE->node.padding_widget_width;
  this->node_width = std::max(GN_STYLE->node.width, (float)min_from_widget);
}

void GraphicsNodeGeometry::compute_ports(const QFontMetrics &fm)
{
  float y = this->header_rect.bottom() + 2.f * GN_STYLE->node.padding;
  float diameter = 2.f * GN_STYLE->node.port_radius;
  float label_x = this->margin + 3.f * GN_STYLE->node.padding;
  float label_w = this->node_width - 6.f * GN_STYLE->node.padding;

  // Hitbox geometry: horizontally extended, vertically clamped to prevent overlap
  float hit_radius = GN_STYLE->node.port_radius * GN_STYLE->node.port_hit_radius_scale;
  float hit_w = 2.f * hit_radius;                                          // full horizontal reach
  float half_hit_h = std::min(hit_radius, (this->line_height - 2.f) * 0.5f); // clamp vertical

  for (int i = 0; i < this->p_node_proxy->get_nports(); i++)
  {
    this->port_label_rects.emplace_back(QRectF(label_x, y, label_w, this->line_height));

    float cy = y + 0.5f * fm.height() - GN_STYLE->node.port_radius;

    bool is_in = (this->p_node_proxy->get_port_type(i) == PortType::IN);

    float cx = is_in ? this->margin - GN_STYLE->node.port_radius
                     : this->margin + this->node_width - GN_STYLE->node.port_radius;

    this->port_rects.emplace_back(QRectF(cx, cy, diameter, diameter));

    // Hitbox: horizontal capsule shape — extends outward (toward wires),
    // vertically clamped so adjacent ports never overlap.
    QPointF center(cx + GN_STYLE->node.port_radius, cy + GN_STYLE->node.port_radius);
    float   hit_x = is_in ? (center.x() - hit_w * 0.7f)   // IN: reach left toward wires
                          : (center.x() - hit_w * 0.3f);   // OUT: reach right toward wires

    this->port_hit_rects.emplace_back(
        QRectF(hit_x, center.y() - half_hit_h, hit_w, 2.f * half_hit_h));

    y += this->line_height;
  }

  this->ports_end_y = y;
}

void GraphicsNodeGeometry::compute_widget_position()
{
  this->widget_pos = QPointF(this->margin + GN_STYLE->node.padding_widget_width,
                             this->ports_end_y + GN_STYLE->node.padding_widget_height);
}

} // namespace gngui
