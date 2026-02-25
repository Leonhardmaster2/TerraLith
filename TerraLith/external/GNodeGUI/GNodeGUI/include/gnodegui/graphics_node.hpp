/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <functional>
#include <memory>
#include <unordered_set>

#include <QEvent>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QPointer>
#include <QWidget>

#include "nlohmann/json.hpp"

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node_geometry.hpp"
#include "gnodegui/logger.hpp"
#include "gnodegui/node_proxy.hpp"

namespace gngui
{

class GraphicsLink; // forward decl

class GraphicsNode : public QGraphicsRectItem
{
public:
  GraphicsNode(QPointer<NodeProxy> p_proxy, QGraphicsItem *parent = nullptr);
  ~GraphicsNode();

  // --- Serializzation

  void           json_from(const nlohmann::json &json);
  nlohmann::json json_to() const;

  // --- Getters

  std::string                 get_caption() const;
  std::string                 get_category() const;
  std::vector<std::string>    get_category_splitted(char delimiter = '/') const;
  std::string                 get_data_type(int port_index) const;
  const GraphicsNodeGeometry &get_geometry() const;
  std::string                 get_id() const;
  std::string                 get_main_category() const;
  int                         get_nports() const;
  std::string                 get_port_caption(int port_index) const;
  std::string                 get_port_id(int port_index) const;
  int                         get_port_index(const std::string &id) const;
  PortType                    get_port_type(int port_index) const;
  const NodeProxy            *get_proxy_ref() const;
  bool                        is_port_available(int port_index);
  bool is_port_compatible(int            port_index,
                          const PortType &source_type,
                          const std::string &source_data_type) const;

  // --- Setters

  void set_is_node_pinned(bool new_state);
  void set_is_port_connected(int port_index, GraphicsLink *p_link);
  void set_p_proxy(QPointer<NodeProxy> new_p_proxy);

  // --- Execution feedback ---
  void  set_last_execution_time(float time_ms);
  float get_last_execution_time() const;
  void  set_build_progress(int percent);
  int   get_build_progress() const;
  void  set_last_backend_type(int backend_type);
  int   get_last_backend_type() const;

  // Fast link tracking for O(K) update_links
  void track_link(GraphicsLink *p_link);
  void untrack_link(GraphicsLink *p_link);

  void set_widget(QWidget *new_widget, QSize widget_size = QSize());
  void set_widget_visibility(bool is_visible);

  // --- UI

  void update_geometry();

  // --- "slots" equivalent
  void on_compute_finished();
  void on_compute_started();

  // --- Callbacks - "signals" equivalent
  std::function<void(GraphicsNode *from, int port_index, QPointF scene_pos)>
      connection_dropped;
  std::function<
      void(GraphicsNode *from, int port_from_index, GraphicsNode *to, int port_to_index)>
                                                                connection_finished;
  std::function<void(GraphicsNode *from, int port_index)>       connection_started;
  std::function<void(const std::string &id)>                    selected;
  std::function<void(const std::string &id)>                    deselected;
  std::function<void(const std::string &id, QPointF scene_pos)> right_clicked;

  // Auto-wiring: called when node is dropped on a link after drag
  std::function<void(GraphicsNode *dropped_node, GraphicsLink *target_link)>
      node_dropped_on_link;

  // Alt+click: disconnect the link on a connected port
  std::function<void(GraphicsLink *link)> disconnect_link;

  // Ctrl+drag: start rerouting a connection from the anchor (other) end
  std::function<void(GraphicsNode *anchor_node, int anchor_port, GraphicsLink *link)>
      reroute_started;

protected:
  // --- Qt methods override

  void     hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  void     hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
  void     mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void     mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void     mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  bool     sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

  virtual void paint(QPainter                       *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget                        *widget) override;

private:
  QSizeF get_widget_size() const;

  // --- Hover state

  int  get_hovered_port_index() const;
  bool update_is_port_hovered(QPointF scene_pos);
  void update_links();
  void reset_is_port_hovered();

  // --- Members

  QPointer<NodeProxy>         p_proxy;
  GraphicsNodeGeometry        geometry;
  std::string                 current_comment;
  QSizeF                      current_widget_size;
  bool                        is_node_dragged = false;
  bool                        is_node_hovered = false;
  bool                        is_node_pinned = false;
  std::vector<bool>           is_port_hovered;
  std::vector<GraphicsLink *>          connected_link_ref; // owned by GraphViewer
  std::unordered_set<GraphicsLink *>   all_connected_links; // fast O(K) link update
  bool                        is_node_computing = false;
  bool                        is_widget_visible = true;
  bool                        has_connection_started = false;
  int                         port_index_from;
  std::string                 data_type_connecting = "";
  PortType                    port_type_connecting = PortType::OUT;
  GraphicsLink               *highlighted_drop_link = nullptr; // for auto-wire preview
  bool                        is_rerouting = false;
  GraphicsNode               *reroute_anchor_node = nullptr;
  int                         reroute_anchor_port = -1;
  QGraphicsProxyWidget       *proxy_widget = nullptr; // owned by this

  // Execution feedback (set by application layer, rendered in paint)
  float                       last_execution_time_ms_ = 0.0f;
  int                         build_progress_percent_ = 0; // 0=idle, 1-99=in progress, 100=done
  int                         last_backend_type_ = 0; // 0=None, 1=CPU, 2=Vulkan, 3=OpenCL
};

// --- helper

bool is_valid(GraphicsNode *node);
bool is_valid(GraphicsLink *link);

} // namespace gngui
