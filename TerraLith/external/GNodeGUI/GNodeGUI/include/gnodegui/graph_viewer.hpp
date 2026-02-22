/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <functional>
#include <unordered_map>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QJsonObject>
#include <QTimer>

#include "nlohmann/json.hpp"

#include "gnodegui/graphics_link.hpp"
#include "gnodegui/graphics_node.hpp"
#include "gnodegui/node_proxy.hpp"

namespace gngui
{

class GraphViewer : public QGraphicsView
{
  Q_OBJECT

public:
  explicit GraphViewer(std::string id = "graph", QWidget *parent = nullptr);

  // --- Serializzation

  void           json_from(nlohmann::json json, bool clear_existing_content = true);
  nlohmann::json json_to() const;

  // --- Add

  void        add_item(QGraphicsItem *item, QPointF scene_pos = QPointF(0.f, 0.f));
  void        add_link(const std::string &id_out,
                       const std::string &port_id_out,
                       const std::string &to_in,
                       const std::string &port_id_in);
  std::string add_node(NodeProxy         *p_node_proxy,
                       QPointF            scene_pos,
                       const std::string &node_id = "");
  void add_static_item(QGraphicsItem *item, QPoint window_pos, float z_value = 0.f);

  // --- Remove

  void clear();
  void remove_node(const std::string &node_id);

  // --- Editing

  void                     deselect_all();
  std::vector<std::string> get_selected_node_ids(
      std::vector<QPointF> *p_scene_pos_list = nullptr);
  void select_all();
  void set_node_as_selected(const std::string &node_id);
  void unpin_nodes();

  // --- UI

  void add_toolbar(QPoint window_pos);
  bool execute_new_node_context_menu();
  void toggle_link_type();
  void zoom_to_content();
  void zoom_to_selection();

  // --- Getters

  QRectF        get_bounding_box();
  GraphicsNode *get_graphics_node_by_id(const std::string &node_id);
  std::string   get_id() const;
  QPointF       get_mouse_scene_pos();

  // --- Setters

  void set_enabled(bool state);
  void set_id(const std::string &new_id) { this->id = new_id; }
  void set_node_inventory(const std::map<std::string, std::string> &new_node_inventory);

  // --- Export

  // useful for debugging graph actual state, after export: to convert, command line: dot
  // export.dot -Tsvg > output.svg
  void export_to_graphviz(const std::string &fname = "export.dot");
  void save_screenshot(const std::string &fname = "screenshot.png");

public Q_SLOTS:

  // --- Qt slots

  void on_compute_finished(const std::string &node_id);
  void on_compute_started(const std::string &node_id);
  void on_node_reload_request(const std::string &node_id);
  void on_node_settings_request(const std::string &node_id);
  void on_node_right_clicked(const std::string &node_id, QPointF scene_pos);
  void on_update_finished();
  void on_update_started();

Q_SIGNALS:

  // --- Link signals

  void connection_deleted(const std::string &id_out,
                          const std::string &port_id_out,
                          const std::string &to_in,
                          const std::string &port_id_in,
                          bool               link_will_be_replaced);
  void connection_dropped(const std::string &node_id,
                          const std::string &port_id,
                          QPointF            scene_pos);
  void connection_finished(const std::string &id_out,
                           const std::string &port_id_out,
                           const std::string &to_in,
                           const std::string &port_id_in);
  void connection_started(const std::string &id_from, const std::string &port_id_from);

  // --- Graph signals

  void graph_automatic_node_layout_request();
  void graph_clear_request();
  void graph_import_request();
  void graph_load_request();
  void graph_new_request();
  void graph_reload_request();
  void graph_save_as_request();
  void graph_save_request();
  void graph_settings_request();

  // --- Node signals

  void new_graphics_node_request(const std::string &node_id, QPointF scene_pos);
  void new_node_request(const std::string &type, QPointF scene_pos);
  void node_deleted(const std::string &node_id);
  void node_deselected(const std::string &node_id);
  void node_reload_request(const std::string &node_id);
  void node_selected(const std::string &node_id);
  void node_settings_request(const std::string &node_id);
  void node_right_clicked(const std::string &node_id, QPointF scene_pos);
  void nodes_copy_request(const std::vector<std::string> &id_list,
                          const std::vector<QPointF>     &scene_pos_list);
  void nodes_duplicate_request(const std::vector<std::string> &id_list,
                               const std::vector<QPointF>     &scene_pos_list);
  void nodes_paste_request();

  // --- Global signals

  void quit_request();
  void selection_has_changed();
  void viewport_request();
  void rubber_band_selection_started();
  void rubber_band_selection_finished();

  // Auto-wiring: node dropped onto a link
  void node_dropped_on_link_request(const std::string &dropped_node_id,
                                    const std::string &link_out_id,
                                    const std::string &link_out_port_id,
                                    const std::string &link_in_id,
                                    const std::string &link_in_port_id);

  // Undo/Redo
  void undo_request();
  void redo_request();

protected:
  // --- Qt events

  void contextMenuEvent(QContextMenuEvent *event) override;
  void delete_selected_items();
  void drawForeground(QPainter *painter, const QRectF &rect) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  // Accessible to subclasses for auto-wiring
  void delete_graphics_link(GraphicsLink *, bool prevent_graph_update = false);
  void delete_graphics_node(GraphicsNode *p_node);
  bool is_item_static(QGraphicsItem *item);

private Q_SLOTS:
  void on_connection_dropped(GraphicsNode *from, int port_index, QPointF scene_pos);

  // reordered: 'from' is 'output' and 'to' is 'input'
  void on_connection_finished(GraphicsNode *from_node,
                              int           port_from_index,
                              GraphicsNode *to_node,
                              int           port_to_index);

  void on_connection_started(GraphicsNode *from_node, int port_index);

private:

  // --- Members

  std::string id;

  std::vector<QGraphicsItem *> static_items; // owned by this
  std::vector<QPoint>          static_items_positions;

  // all nodes available store as a map of (node type, node category)
  std::map<std::string, std::string> node_inventory;

  GraphicsLink *temp_link = nullptr;   // Temporary link
  GraphicsNode *source_node = nullptr; // Source node for the connection
  LinkType      current_link_type = LinkType::CUBIC;

  // Middle-mouse panning state
  bool   is_panning = false;
  QPoint pan_last_pos;

  // O(1) node lookup index (replaces linear scene scans)
  std::unordered_map<std::string, GraphicsNode *> node_index_;

  // Zoom limits
  static constexpr float zoom_min_ = 0.3f;
  static constexpr float zoom_max_ = 5.0f;

  // Drag pulse timer for port compatibility animation + edge panning
  QTimer *drag_pulse_timer_ = nullptr;

  // Edge-pan parameters (auto-scroll when dragging near viewport edges)
  static constexpr int   edge_pan_margin_ = 40;   // pixels from edge to trigger
  static constexpr float edge_pan_speed_  = 8.0f; // pixels per tick at edge
};

} // namespace gngui
