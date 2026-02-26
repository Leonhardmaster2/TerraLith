/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <functional>
#include <map>
#include <memory>

#include <QScrollArea>
#include <QThread>
#include <QUndoStack>

#include "gnodegui/graph_viewer.hpp"

#include "hesiod/model/nodes/base_node.hpp"

namespace hesiod
{

class GraphNode;   // forward
class GraphWorker; // forward

// =====================================
// GraphNodeWidget
// =====================================
class GraphNodeWidget : public gngui::GraphViewer
{
  Q_OBJECT

public:
  // --- Constructor and Setup ---
  GraphNodeWidget(std::weak_ptr<GraphNode> p_graph_node, QWidget *parent = nullptr);
  ~GraphNodeWidget();

  void automatic_node_layout();
  void clear_all();
  void clear_data_viewers();
  void clear_graphic_scene();
  void setup_connections();

  // --- Serialization ---
  void           json_from(nlohmann::json const &json);
  nlohmann::json json_import(nlohmann::json const &json,
                             QPointF               scene_pos = QPointF(0.f, 0.f));
  nlohmann::json json_to() const;

  bool       get_is_selecting_with_rubber_band() const;
  GraphNode *get_p_graph_node();
  void       set_json_copy_buffer(nlohmann::json const &new_json_copy_buffer);

  void add_import_texture_nodes(const std::vector<std::string> &texture_paths);

  // --- Background Compute ---
  bool is_computing() const;
  void force_build();

signals:
  // TODO REMOVE GRAPH_ID

  // --- User Actions Signals ---
  void copy_buffer_has_changed(const nlohmann::json &new_json);
  void has_been_cleared(const std::string &graph_id);
  void new_node_created(const std::string &graph_id, const std::string &id);
  void node_deleted(const std::string &graph_id, const std::string &id);

  // --- Graph update ---
  void compute_started(const std::string &node_id);
  void compute_finished(const std::string &node_id);
  void update_started();
  void update_finished();
  void update_progress(const std::string &node_id, float progress);

public slots:
  void closeEvent(QCloseEvent *event) override;

  // --- User Actions ---
  void on_connection_deleted(const std::string &id_out,
                             const std::string &port_id_out,
                             const std::string &id_in,
                             const std::string &port_id_in,
                             bool               prevent_graph_update);
  void on_connection_dropped(const std::string &node_id,
                             const std::string &port_id,
                             QPointF            scene_pos);
  void on_connection_finished(const std::string &id_out,
                              const std::string &port_id_out,
                              const std::string &id_in,
                              const std::string &port_id_in);

  void on_graph_clear_request();
  void on_graph_import_request();
  void on_graph_new_request();
  void on_graph_reload_request();
  void on_graph_settings_request();

  std::string on_new_node_request(const std::string &node_type, QPointF scene_pos);
  void        on_node_deleted_request(const std::string &node_id);
  void        on_node_reload_request(const std::string &node_id);
  void        on_node_right_clicked(const std::string &node_id, QPointF scene_pos);

  void on_nodes_copy_request(const std::vector<std::string> &id_list,
                             const std::vector<QPointF>     &scene_pos_list);
  void on_nodes_duplicate_request(const std::vector<std::string> &id_list,
                                  const std::vector<QPointF>     &scene_pos_list);
  void on_nodes_paste_request();

  void on_node_dropped_on_link(const std::string &dropped_node_id,
                               const std::string &link_out_id,
                               const std::string &link_out_port_id,
                               const std::string &link_in_id,
                               const std::string &link_in_port_id);

  void on_node_info(const std::string &node_id);
  void on_node_pinned(const std::string &node_id, bool state);
  void on_viewport_request();

  // Used by undo commands to re-delete nodes during redo
  void delete_nodes_by_ids(const std::vector<std::string> &ids);

  // --- Undo command helpers (called by undo command classes) ---
  void restore_nodes_from_snapshot(const nlohmann::json &snapshot);
  void create_node_with_id(const std::string  &node_type,
                           const std::string  &node_id,
                           QPointF             scene_pos,
                           const nlohmann::json &settings);
  void create_link_internal(const std::string &id_out,
                            const std::string &port_id_out,
                            const std::string &id_in,
                            const std::string &port_id_in);
  void remove_link_internal(const std::string &id_out,
                            const std::string &port_id_out,
                            const std::string &id_in,
                            const std::string &port_id_in);
  void restore_node_attributes(const std::string    &node_id,
                               const nlohmann::json &attrs_json);
  nlohmann::json build_nodes_snapshot(const std::vector<std::string> &node_ids);

  // --- Others... ---
  void on_new_graphics_node_request(const std::string &node_id, QPointF scene_pos);

  // --- Undo / Redo ---
  void        on_undo_request();
  void        on_redo_request();
  QUndoStack *get_undo_stack() { return this->undo_stack; }

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
  // --- Background Compute Slots ---
  void on_worker_node_compute_started(const std::string &node_id);
  void on_worker_node_compute_finished(const std::string &node_id);
  void on_worker_progress_updated(const std::string &node_id, float percent);
  void on_worker_node_execution_time(const std::string &node_id, float time_ms, int backend_type);
  void on_worker_compute_all_finished(bool was_cancelled);

private:
  QScrollArea *create_attributes_scroll(QWidget *parent, QWidget *attr_widget);

  void backup_selected_ids();
  void reselect_backup_ids();
  void delete_selected_with_undo();

  // --- Background Compute ---
  void start_background_compute(const std::vector<std::string> &sorted_ids);
  void cancel_background_compute();

  // --- Members ---
  std::weak_ptr<GraphNode>       p_graph_node; // own by GraphManager
  std::vector<QPointer<QWidget>> data_viewers;
  bool                           update_node_on_connection_finished = true;
  nlohmann::json                 json_copy_buffer;
  std::string                    last_node_created_id = "";
  bool                           is_selecting_with_rubber_band = false;
  std::filesystem::path          last_import_path;
  std::vector<std::string>       selected_ids;

  // Undo / Redo
  QUndoStack                        *undo_stack = nullptr;
  std::map<std::string, QPointF>     drag_start_positions_;
  bool                               suppress_undo_ = false; // when true, skip pushing undo commands

  // Background compute
  QThread     *worker_thread_ = nullptr;
  GraphWorker *graph_worker_  = nullptr;
  bool         is_computing_  = false;

  // Saved callbacks (suppressed during background compute)
  std::function<void(const std::string &)>              saved_compute_started_;
  std::function<void(const std::string &)>              saved_compute_finished_;
  std::function<void()>                                 saved_update_started_;
  std::function<void()>                                 saved_update_finished_;
  std::function<void(const std::string &, float)>       saved_update_progress_;
};

} // namespace hesiod