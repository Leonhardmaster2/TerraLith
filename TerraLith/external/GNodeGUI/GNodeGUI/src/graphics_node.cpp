/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <cmath>
#include <sstream>

#include <QApplication>
#include <QDateTime>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node.hpp"
#include "gnodegui/icons/reload_icon.hpp"
#include "gnodegui/icons/show_settings_icon.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/style.hpp"
#include "gnodegui/utils.hpp"

namespace gngui
{

GraphicsNode::GraphicsNode(QPointer<NodeProxy> p_proxy, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), p_proxy(p_proxy)
{
  if (!this->p_proxy)
  {
    Logger::log()->error("GraphicsNode::GraphicsNode: input p_proxy is nullptr");
    return;
  }

  // item flags
  this->setFlag(QGraphicsItem::ItemIsSelectable, true);
  this->setFlag(QGraphicsItem::ItemIsMovable, true);
  this->setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, false);
  this->setFlag(QGraphicsItem::ItemIsFocusable, true);
  this->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
  this->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

  this->setAcceptHoverEvents(true);
  this->setOpacity(1.f);
  this->setZValue(0);

  // performance: use device coordinate cache instead of QGraphicsDropShadowEffect
  // (drop shadow effects are extremely expensive on moving items)
  this->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  // tooltip
  const std::string tooltip = this->p_proxy->get_tool_tip_text();
  if (!tooltip.empty())
    this->setToolTip(QString::fromStdString(tooltip));

  // initialize port states
  this->is_port_hovered.resize(this->get_nports());
  this->connected_link_ref.resize(this->get_nports());

  // geometry
  this->update_geometry();
}

GraphicsNode::~GraphicsNode()
{
  Logger::log()->debug("GraphicsNode::~GraphicsNode: {}", this->get_id());

  // stop interactions
  this->setEnabled(false);
  this->setAcceptHoverEvents(false);
  this->setAcceptedMouseButtons(Qt::NoButton);

  // destroy proxy widget safely
  if (this->proxy_widget)
  {
    this->proxy_widget->setWidget(nullptr);
    this->proxy_widget->deleteLater();
    this->proxy_widget = nullptr;
  }
}

std::string GraphicsNode::get_caption() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_caption();
}

std::string GraphicsNode::get_category() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_category();
}

std::vector<std::string> GraphicsNode::get_category_splitted(char delimiter) const
{
  return split_string(this->get_category(), delimiter);
}

std::string GraphicsNode::get_data_type(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_data_type(port_index);
}

const GraphicsNodeGeometry &GraphicsNode::get_geometry() const { return this->geometry; }

int GraphicsNode::get_hovered_port_index() const
{
  auto it = std::find(this->is_port_hovered.begin(), this->is_port_hovered.end(), true);

  // if found, calculate the index; otherwise, return -1
  if (it != this->is_port_hovered.end())
  {
    int index = std::distance(this->is_port_hovered.begin(), it);
    return index;
  }
  else
    return -1;
}

std::string GraphicsNode::get_id() const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_id();
}

std::string GraphicsNode::get_main_category() const
{
  std::string node_category = this->get_category();
  int         pos = node_category.find("/");
  return node_category.substr(0, pos);
}

int GraphicsNode::get_nports() const
{
  if (!this->p_proxy)
    return 0;

  return this->p_proxy->get_nports();
}

std::string GraphicsNode::get_port_caption(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_port_caption(port_index);
}

std::string GraphicsNode::get_port_id(int port_index) const
{
  if (!this->p_proxy)
    return std::string();

  return this->p_proxy->get_port_id(port_index);
}

int GraphicsNode::get_port_index(const std::string &id) const
{
  for (int k = 0; k < this->get_nports(); k++)
    if (this->get_port_id(k) == id)
      return k;

  return -1;
}

PortType GraphicsNode::get_port_type(int port_index) const
{
  if (!this->p_proxy)
    return PortType::OUT;

  return this->p_proxy->get_port_type(port_index);
}

const NodeProxy *GraphicsNode::get_proxy_ref() const { return this->p_proxy; }

QSizeF GraphicsNode::get_widget_size() const
{
  QSizeF size = QSizeF();

  if (this->proxy_widget)
  {
    if (QWidget *widget = this->proxy_widget->widget())
      size = widget->size();
  }

  return size;
}

void GraphicsNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_node_hovered = true;

  // Elevate Z so this node's ports render above adjacent nodes
  this->setZValue(1);

  this->update();
  QGraphicsRectItem::hoverEnterEvent(event);
}

void GraphicsNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  this->is_node_hovered = false;
  this->setCursor(Qt::ArrowCursor);

  // Flush all port hover states to prevent sticky glow
  this->reset_is_port_hovered();

  // Restore Z to default
  this->setZValue(0);

  this->update();
  QGraphicsRectItem::hoverLeaveEvent(event);
}

void GraphicsNode::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
  QPointF pos = event->pos();
  QPointF scene_pos = this->mapToScene(pos);
  QPointF item_pos = scene_pos - this->scenePos();

  if (this->update_is_port_hovered(item_pos))
    this->update();

  QGraphicsRectItem::hoverMoveEvent(event);
}

bool GraphicsNode::is_port_available(int port_index)
{
  return this->get_port_type(port_index) == PortType::OUT ||
         !this->connected_link_ref[port_index];
}

bool GraphicsNode::is_port_compatible(int                port_index,
                                      const PortType    &source_type,
                                      const std::string &source_data_type) const
{
  // Compatible if: opposite port direction AND matching data types
  return this->get_port_type(port_index) != source_type &&
         this->get_data_type(port_index) == source_data_type;
}

QVariant GraphicsNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == QGraphicsItem::ItemSelectedHasChanged)
  {
    const bool new_selection_state = value.toBool();

    if (new_selection_state)
    {
      if (this->selected)
        this->selected(this->get_id());
    }
    else
    {
      if (this->deselected)
        this->deselected(this->get_id());
    }
  }

  if (change == QGraphicsItem::ItemPositionHasChanged)
  {
    this->update_links();
  }

  return QGraphicsItem::itemChange(change, value);
}

void GraphicsNode::json_from(const nlohmann::json &json)
{
  json_safe_get(json, "is_widget_visible", this->is_widget_visible);

  float x = 0;
  float y = 0;
  json_safe_get(json, "scene_position.x", x);
  json_safe_get(json, "scene_position.y", y);
  this->setPos(QPointF(x, y));
}

nlohmann::json GraphicsNode::json_to() const
{
  nlohmann::json json;

  json["is_widget_visible"] = this->is_widget_visible;
  json["scene_position.x"] = this->scenePos().x();
  json["scene_position.y"] = this->scenePos().y();

  // for info only
  {
    json["id"] = this->get_id();
    json["caption"] = this->get_caption();
  }

  return json;
}

void GraphicsNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  // let the base class handle normal movement
  QGraphicsItem::mouseMoveEvent(event);

  // Auto-wire highlight: show which link the node would be inserted into
  if (this->is_node_dragged && this->node_dropped_on_link && this->scene())
  {
    GraphicsLink *best_link = nullptr;

    for (QGraphicsItem *item : this->collidingItems(Qt::IntersectsItemShape))
    {
      if (GraphicsLink *p_link = dynamic_cast<GraphicsLink *>(item))
      {
        // Skip links already connected to this node to prevent self-wiring
        if (this->all_connected_links.count(p_link))
          continue;
        best_link = p_link;
        break;
      }
    }

    // Update highlight state
    if (best_link != this->highlighted_drop_link)
    {
      if (this->highlighted_drop_link)
        this->highlighted_drop_link->set_is_drop_target(false);

      this->highlighted_drop_link = best_link;

      if (this->highlighted_drop_link)
        this->highlighted_drop_link->set_is_drop_target(true);
    }
  }
}

void GraphicsNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    int hovered_port_index = this->get_hovered_port_index();

    if (hovered_port_index >= 0)
    {
      Logger::log()->trace("GraphicsNode::mousePressEvent: connection_started {}:{}",
                           this->get_id(),
                           hovered_port_index);

      this->has_connection_started = true;
      this->setFlag(QGraphicsItem::ItemIsMovable, false);
      this->port_index_from = hovered_port_index;
      this->data_type_connecting = this->get_data_type(hovered_port_index);
      this->port_type_connecting = this->get_port_type(hovered_port_index);
      if (this->connection_started)
        this->connection_started(this, hovered_port_index);
      event->accept();
    }
    else
    {
      this->is_node_dragged = true;
    }
  }

  else if (event->button() == Qt::RightButton)
  {
    QPointF pos = event->pos();
    QPointF scene_pos = this->mapToScene(pos);
    if (this->right_clicked)
      this->right_clicked(this->get_id(), scene_pos);
  }

  QGraphicsRectItem::mousePressEvent(event);
}

void GraphicsNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    if (this->is_node_dragged)
    {
      this->is_node_dragged = false;

      // Clear any auto-wire highlight
      if (this->highlighted_drop_link)
      {
        this->highlighted_drop_link->set_is_drop_target(false);
        this->highlighted_drop_link = nullptr;
      }

      // Check for auto-wiring: was node dropped on a link?
      if (this->node_dropped_on_link && this->scene())
      {
        for (QGraphicsItem *item : this->collidingItems(Qt::IntersectsItemShape))
        {
          if (GraphicsLink *p_link = dynamic_cast<GraphicsLink *>(item))
          {
            // Skip links already connected to this node to prevent self-wiring
            if (this->all_connected_links.count(p_link))
              continue;
            this->node_dropped_on_link(this, p_link);
            break; // Only wire into first link found
          }
        }
      }
    }
    else if (this->has_connection_started)
    {
      bool                   is_dropped = true;
      QList<QGraphicsItem *> items_under_mouse = scene()->items(event->scenePos());

      for (QGraphicsItem *item : items_under_mouse)
      {
        if (GraphicsNode *p_target_node = dynamic_cast<GraphicsNode *>(item))
        {

          int hovered_port_index = p_target_node ? p_target_node->get_hovered_port_index()
                                                 : -1;
          if (hovered_port_index >= 0 && p_target_node != this)
          {
            Logger::log()->trace(
                "GraphicsNode::mouseReleaseEvent: connection_finished {}:{}",
                p_target_node->get_id(),
                hovered_port_index);

            if (this->connection_finished)
              this->connection_finished(this,
                                        this->port_index_from,
                                        p_target_node,
                                        hovered_port_index);

            is_dropped = false;
            break;
          }
          else
          {
            is_dropped = true;
            break;
          }
        }
      }

      this->reset_is_port_hovered();
      this->update();

      if (is_dropped)
      {
        Logger::log()->trace("GraphicsNode::mouseReleaseEvent connection_dropped {}",
                             this->get_id());

        if (this->connection_dropped)
          this->connection_dropped(this, this->port_index_from, event->scenePos());
      }

      this->has_connection_started = false;

      // clean-up port color state
      for (QGraphicsItem *item : this->scene()->items())
      {
        if (GraphicsNode *node = dynamic_cast<GraphicsNode *>(item))
        {
          node->data_type_connecting = "";
          node->port_type_connecting = PortType::OUT;
          node->update();
        }
      }

      this->setFlag(QGraphicsItem::ItemIsMovable, true);
    }
  }

  QGraphicsRectItem::mouseReleaseEvent(event);
}

void GraphicsNode::on_compute_finished()
{
  Logger::log()->trace("GraphicsNode::on_compute_finished, node {}", this->get_caption());
  this->is_node_computing = false;
  this->update();
}

void GraphicsNode::on_compute_started()
{
  Logger::log()->trace("GraphicsNode::on_compute_started, node {}", this->get_caption());
  this->is_node_computing = true;
  this->update();
}

void GraphicsNode::paint(QPainter                       *painter,
                         const QStyleOptionGraphicsItem * /* option */,
                         QWidget * /* widget */)
{
  if (!this->p_proxy)
    return;

  if (current_widget_size != this->get_widget_size())
    this->update_geometry();

  painter->save();

  // --- LOD-based smooth fade: details fade out between 0.25x and 0.5x zoom
  const float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(
      painter->worldTransform());
  const float detail_alpha = std::clamp((lod - 0.25f) / 0.25f, 0.0f, 1.0f);

  // --- Painted fake shadow (replaces QGraphicsDropShadowEffect for performance)
  {
    QRectF shadow_rect = this->geometry.body_rect.translated(2.0, 2.0);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 50));
    painter->drawRoundedRect(shadow_rect,
                             GN_STYLE->node.rounding_radius,
                             GN_STYLE->node.rounding_radius);
  }

  // --- Background rectangle (always fully opaque)

  painter->setBrush(QBrush(GN_STYLE->node.color_bg));
  painter->setPen(Qt::NoPen);
  painter->drawRoundedRect(this->geometry.body_rect,
                           GN_STYLE->node.rounding_radius,
                           GN_STYLE->node.rounding_radius);

  // --- Fully zoomed out: draw minimal border only to save CPU
  if (detail_alpha <= 0.0f)
  {
    painter->setBrush(Qt::NoBrush);
    if (this->isSelected())
      painter->setPen(
          QPen(GN_STYLE->node.color_selected, GN_STYLE->node.pen_width_selected));
    else
      painter->setPen(QPen(GN_STYLE->node.color_border, GN_STYLE->node.pen_width));

    painter->drawRoundedRect(this->geometry.body_rect,
                             GN_STYLE->node.rounding_radius,
                             GN_STYLE->node.rounding_radius);

    painter->restore();
    return;
  }

  // --- Apply detail fade for all subsequent drawing
  painter->setOpacity(detail_alpha);

  // --- Header background (category-tinted, clipped to rounded top corners)

  {
    std::string main_category = this->get_main_category();
    QColor      header_color = GN_STYLE->node.color_bg_light;

    if (GN_STYLE->node.color_category.contains(main_category))
      header_color = GN_STYLE->node.color_category.at(main_category);

    painter->setPen(Qt::NoPen);
    QRectF rect = this->geometry.header_rect;
    float  radius = GN_STYLE->node.rounding_radius;

    // Clipped path: rounded top corners, flat bottom edge
    QPainterPath header_path;
    header_path.moveTo(rect.left(), rect.bottom());
    header_path.lineTo(rect.left(), rect.top() + radius);
    header_path.arcTo(rect.left(), rect.top(), radius * 2, radius * 2, 180, -90);
    header_path.lineTo(rect.right() - radius, rect.top());
    header_path.arcTo(rect.right() - radius * 2, rect.top(), radius * 2, radius * 2, 90, -90);
    header_path.lineTo(rect.right(), rect.bottom());
    header_path.closeSubpath();

    // Subtle category-tinted gradient over the dark header base
    QColor top_color = header_color;
    QColor bot_color = header_color;

    if (this->is_node_computing)
    {
      top_color.setAlpha(60);
      bot_color.setAlpha(30);
    }
    else
    {
      top_color = header_color.lighter(115);
      top_color.setAlpha(140);
      bot_color.setAlpha(70);
    }

    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0, top_color);
    gradient.setColorAt(1.0, bot_color);
    painter->setBrush(gradient);
    painter->drawPath(header_path);

    // Thin separator line between header and body
    painter->setPen(QPen(QColor(255, 255, 255, 25), 1.0f));
    painter->drawLine(QPointF(rect.left(), rect.bottom()),
                      QPointF(rect.right(), rect.bottom()));
  }

  // --- Title text (bold, #E0E2E8)

  {
    QFont bold_font = painter->font();
    bold_font.setBold(true);
    painter->setFont(bold_font);

    painter->setPen(this->isSelected() ? GN_STYLE->node.color_selected
                                       : GN_STYLE->node.color_caption);
    painter->drawText(this->geometry.caption_pos, this->get_caption().c_str());

    // Restore normal font
    bold_font.setBold(false);
    painter->setFont(bold_font);
  }

  // --- Category subtitle (dim, #80838D)

  {
    painter->setPen(GN_STYLE->node.color_caption_dim);
    painter->drawText(this->geometry.category_pos,
                      this->get_main_category().c_str());
  }

  // --- Pinned node outer border

  if (this->is_node_pinned)
  {
    painter->setBrush(Qt::NoBrush);

    QPen pen = QPen(GN_STYLE->node.color_pinned, 2.f * GN_STYLE->node.pen_width_selected);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);

    float w = GN_STYLE->node.pen_width_selected;

    painter->drawRoundedRect(this->geometry.body_rect.adjusted(-w, -w, w, w),
                             GN_STYLE->node.rounding_radius,
                             GN_STYLE->node.rounding_radius);

    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
  }

  // --- Border

  painter->setBrush(Qt::NoBrush);
  if (this->isSelected())
    painter->setPen(
        QPen(GN_STYLE->node.color_selected, GN_STYLE->node.pen_width_selected));
  else if (this->is_node_hovered)
    painter->setPen(
        QPen(GN_STYLE->node.color_border_hovered, GN_STYLE->node.pen_width_hovered));
  else
    painter->setPen(QPen(GN_STYLE->node.color_border, GN_STYLE->node.pen_width));

  painter->drawRoundedRect(this->geometry.body_rect,
                           GN_STYLE->node.rounding_radius,
                           GN_STYLE->node.rounding_radius);

  // --- Ports

  // Time-based pulse for compatible ports during connection drag
  const bool   is_dragging = !this->data_type_connecting.empty();
  const double pulse_t = is_dragging
                             ? (QDateTime::currentMSecsSinceEpoch() % 1000) / 1000.0
                             : 0.0;
  // sine wave: 0.0→1.0→0.0 over 1 second
  const float pulse = is_dragging
                          ? 0.5f + 0.5f * static_cast<float>(std::sin(pulse_t * 2.0 * M_PI))
                          : 0.f;

  for (int k = 0; k < this->p_proxy->get_nports(); k++)
  {
    // Resolve semantic color and connection state
    std::string data_type = this->get_data_type(k);
    QColor      semantic_color = get_color_from_data_type(data_type);
    bool        is_connected = (this->connected_link_ref[k] != nullptr);
    bool        is_hovered = this->is_port_hovered[k];
    float       base_radius = GN_STYLE->node.port_radius;
    QPointF     center = this->geometry.port_rects[k].center();

    // Determine compatibility during active drag
    bool is_compatible = is_dragging &&
                         this->is_port_compatible(k,
                                                  this->port_type_connecting,
                                                  this->data_type_connecting);
    bool is_incompatible = is_dragging && !is_compatible;

    // --- Incompatible port: faded to 30% opacity (multiplied by LOD fade)
    if (is_incompatible)
    {
      painter->save();
      painter->setOpacity(0.3f * detail_alpha);

      // Port label (faded)
      int align_flag = (this->get_port_type(k) == PortType::IN) ? Qt::AlignLeft
                                                                 : Qt::AlignRight;
      painter->setPen(Qt::white);
      painter->drawText(this->geometry.port_label_rects[k],
                        align_flag,
                        this->get_port_caption(k).c_str());

      // Port circle (faded)
      if (is_connected)
      {
        painter->setPen(Qt::NoPen);
        painter->setBrush(semantic_color);
      }
      else
      {
        painter->setPen(QPen(semantic_color, GN_STYLE->node.port_stroke_width));
        painter->setBrush(GN_STYLE->node.color_bg);
      }
      painter->drawEllipse(center, base_radius, base_radius);

      painter->restore();
      continue;
    }

    // --- Port label (full opacity)
    int align_flag = (this->get_port_type(k) == PortType::IN) ? Qt::AlignLeft
                                                              : Qt::AlignRight;
    painter->setPen(Qt::white);
    painter->drawText(this->geometry.port_label_rects[k],
                      align_flag,
                      this->get_port_caption(k).c_str());

    float draw_radius = base_radius;

    // --- Compatible port during drag: pulsing glow to attract attention
    if (is_compatible)
    {
      float glow_alpha = 40.f + 80.f * pulse;       // 40→120 oscillation
      float glow_scale = 1.6f + 0.4f * pulse;       // 1.6→2.0 oscillation

      QColor glow_color = semantic_color;
      glow_color.setAlpha(static_cast<int>(glow_alpha));
      painter->setPen(Qt::NoPen);
      painter->setBrush(glow_color);
      painter->drawEllipse(center, base_radius * glow_scale, base_radius * glow_scale);
    }

    // --- Hover state: glow ring + scale up
    if (is_hovered)
    {
      draw_radius = base_radius * GN_STYLE->node.port_hover_visual_scale;

      // Draw semi-transparent glow ring behind the port
      QColor glow_color = semantic_color;
      glow_color.setAlpha(static_cast<int>(GN_STYLE->node.port_glow_alpha));
      painter->setPen(Qt::NoPen);
      painter->setBrush(glow_color);
      float glow_radius = draw_radius * 1.8f;
      painter->drawEllipse(center, glow_radius, glow_radius);
    }

    // --- Draw the port circle
    if (is_connected)
    {
      // Connected: solid fill with semantic color
      painter->setPen(Qt::NoPen);
      painter->setBrush(semantic_color);
    }
    else
    {
      // Disconnected: hollow — node bg fill, semantic stroke
      painter->setPen(QPen(semantic_color, GN_STYLE->node.port_stroke_width));
      painter->setBrush(GN_STYLE->node.color_bg);
    }

    painter->drawEllipse(center, draw_radius, draw_radius);
  }

  // --- Progress bar (slim, at bottom edge of the body)

  if (this->build_progress_percent_ > 0 && this->build_progress_percent_ < 100)
  {
    float bar_height = 3.0f;
    QRectF bar_bg(this->geometry.body_rect.left(),
                  this->geometry.body_rect.bottom() - bar_height,
                  this->geometry.body_rect.width(),
                  bar_height);

    // Background track
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(21, 21, 24, 180));
    painter->drawRect(bar_bg);

    // Progress fill (Primary Accent #4396B2)
    float progress_fraction = this->build_progress_percent_ / 100.0f;
    QRectF bar_fill = bar_bg;
    bar_fill.setWidth(bar_bg.width() * progress_fraction);
    painter->setBrush(QColor(67, 150, 178, 220)); // #4396B2
    painter->drawRect(bar_fill);
  }

  // --- Execution time (dim text, bottom-right of node body)

  if (this->last_execution_time_ms_ > 0.0f)
  {
    QString time_text;
    if (this->last_execution_time_ms_ >= 1000.0f)
      time_text = QString::number(this->last_execution_time_ms_ / 1000.0f, 'f', 1) + " s";
    else
      time_text = QString::number(static_cast<int>(this->last_execution_time_ms_)) + " ms";

    QFont small_font = painter->font();
    small_font.setPointSizeF(small_font.pointSizeF() * 0.8);
    painter->setFont(small_font);
    painter->setPen(QColor(128, 131, 141, 180)); // #80838D dim

    float text_margin = GN_STYLE->node.padding;
    QRectF time_rect(this->geometry.body_rect.left() + text_margin,
                     this->geometry.body_rect.bottom() - this->geometry.line_height -
                         text_margin,
                     this->geometry.body_rect.width() - 2.f * text_margin,
                     this->geometry.line_height);

    painter->drawText(time_rect, Qt::AlignRight | Qt::AlignVCenter, time_text);
  }

  // --- Comment

  std::string comment = this->p_proxy->get_comment();

  if (!comment.empty())
  {
    if (comment != this->current_comment)
      this->update_geometry();

    painter->setPen(GN_STYLE->node.color_comment);
    painter->drawText(this->geometry.comment_rect,
                      Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                      comment.c_str());

    this->current_comment = comment;
  }

  // Restore full opacity so we don't corrupt other items in the scene
  painter->setOpacity(1.0);

  painter->restore();
}

void GraphicsNode::set_build_progress(int percent)
{
  this->build_progress_percent_ = std::clamp(percent, 0, 100);
  this->update();
}

int GraphicsNode::get_build_progress() const
{
  return this->build_progress_percent_;
}

void GraphicsNode::set_last_execution_time(float time_ms)
{
  this->last_execution_time_ms_ = time_ms;
  this->update();
}

float GraphicsNode::get_last_execution_time() const
{
  return this->last_execution_time_ms_;
}

void GraphicsNode::set_is_node_pinned(bool new_state)
{
  this->is_node_pinned = new_state;
  this->update();
}

void GraphicsNode::set_is_port_connected(int port_index, GraphicsLink *p_link)
{
  this->connected_link_ref[port_index] = p_link;
}

void GraphicsNode::track_link(GraphicsLink *p_link)
{
  if (p_link)
    this->all_connected_links.insert(p_link);
}

void GraphicsNode::untrack_link(GraphicsLink *p_link)
{
  if (p_link)
    this->all_connected_links.erase(p_link);
}

void GraphicsNode::reset_is_port_hovered()
{
  this->is_port_hovered.assign(this->is_port_hovered.size(), false);
}

bool GraphicsNode::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
  // Try to cast the watched item to a GraphicsNode
  if (GraphicsNode *p_node = dynamic_cast<GraphicsNode *>(watched))
  {
    // Check for mouse move while connection started
    if (event->type() == QEvent::GraphicsSceneMouseMove && p_node &&
        p_node->has_connection_started)
    {
      QGraphicsSceneMouseEvent *mouse_event = static_cast<QGraphicsSceneMouseEvent *>(
          event);
      QPointF item_pos = mouse_event->scenePos() - this->scenePos();

      // Update current data type of the building connection
      if (p_node && this->data_type_connecting != p_node->data_type_connecting)
      {
        this->data_type_connecting = p_node->data_type_connecting;
        this->port_type_connecting = p_node->port_type_connecting;
        this->update();
      }

      if (!p_node)
        return false; // one got deleted during update

      // Update hovering port status
      if (this->update_is_port_hovered(item_pos))
      {
        for (int k = 0; k < this->get_nports(); k++)
        {
          if (this->is_port_hovered[k])
          {
            if (!p_node)
              break; // avoid deref if deleted mid-loop

            // Use extracted compatibility check
            if (!this->is_port_compatible(k,
                                          p_node->port_type_connecting,
                                          p_node->data_type_connecting))
              this->is_port_hovered[k] = false;
          }
        }
      }
    }
  }

  return QGraphicsRectItem::sceneEventFilter(watched, event);
}

void GraphicsNode::set_p_proxy(QPointer<NodeProxy> new_p_proxy)
{
  this->p_proxy = new_p_proxy;
}

void GraphicsNode::set_widget(QWidget *new_widget, QSize new_widget_size)
{
  Logger::log()->debug("GraphicsNode::set_widget");

  if (!this->p_proxy || !new_widget)
    return;

  // erase current parenting
  if (new_widget->parentWidget())
    new_widget->setParent(nullptr);

  // clean-up existing container
  if (this->proxy_widget)
  {
    QWidget *old = this->proxy_widget->widget();
    this->proxy_widget->setWidget(nullptr);
    if (old)
      old->deleteLater();
    this->proxy_widget->deleteLater();
  }

  // eventually set widget
  this->proxy_widget = new QGraphicsProxyWidget(this);
  this->proxy_widget->setWidget(new_widget);

  if (!new_widget_size.isValid())
    new_widget_size = new_widget->sizeHint();
  this->proxy_widget->resize(new_widget_size);

  // update the geometry
  this->update_geometry();
  this->proxy_widget->setPos(this->geometry.widget_pos);
  this->update();
}

void GraphicsNode::set_widget_visibility(bool is_visible)
{
  if (!this->proxy_widget)
    return;

  QWidget *widget = this->proxy_widget->widget();

  if (!widget)
    return;

  widget->setVisible(is_visible);

  this->update_geometry();
  this->update();
}

void GraphicsNode::update_geometry()
{
  if (!this->p_proxy)
    return;

  // determine widget size (if any)
  QSizeF widget_size = this->get_widget_size();

  // geometry
  this->geometry = GraphicsNodeGeometry(this->p_proxy, widget_size);
  this->setRect(0.f, 0.f, this->geometry.full_width, this->geometry.full_height);
}

bool GraphicsNode::update_is_port_hovered(QPointF item_pos)
{
  // Determine which port (if any) the mouse is over, and whether state changed
  int  new_hover = -1;
  bool had_hover = false;

  for (size_t k = 0; k < this->geometry.port_hit_rects.size(); k++)
  {
    if (this->is_port_hovered[k])
      had_hover = true;
    // pick the first matching hitbox (they should never overlap now)
    if (new_hover < 0 && this->geometry.port_hit_rects[k].contains(item_pos))
      new_hover = static_cast<int>(k);
  }

  // Always clear ALL hover states first to prevent sticky double-hover
  this->reset_is_port_hovered();

  if (new_hover >= 0)
  {
    this->is_port_hovered[new_hover] = true;
    return true; // state changed: now hovering a port
  }

  return had_hover; // state changed only if we were hovering before
}

void GraphicsNode::update_links()
{
  // Use the cached flat set instead of scanning all scene items.
  // This turns O(N) per node move into O(K) where K = connected links.
  for (GraphicsLink *p_link : this->all_connected_links)
    if (p_link)
      p_link->update_path();
}

// --- helper

bool is_valid(GraphicsNode *node) { return node && node->scene() != nullptr; }

bool is_valid(GraphicsLink *link) { return link && link->scene() != nullptr; }

} // namespace gngui
