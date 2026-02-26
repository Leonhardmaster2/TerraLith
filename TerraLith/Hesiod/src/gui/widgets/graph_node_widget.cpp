/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <exception>

#include <QApplication>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QMessageBox>
#include <QScreen>
#include <QTimer>
#include <QToolButton>
#include <QUndoCommand>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "attributes/widgets/abstract_widget.hpp"
#include "attributes/widgets/attributes_widget.hpp"
#include "attributes/widgets/filename_widget.hpp"

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/gui/widgets/custom_qmenu.hpp"
#include "hesiod/gui/widgets/graph_config_dialog.hpp"
#include "hesiod/gui/widgets/graph_node_widget.hpp"
#include "hesiod/gui/widgets/undo_commands.hpp"
#include "hesiod/gui/workers/graph_worker.hpp"
#include "hesiod/gui/widgets/gui_utils.hpp"
#include "hesiod/gui/widgets/node_attributes_widget.hpp"
#include "hesiod/gui/widgets/node_info_dialog.hpp"
#include "hesiod/gui/widgets/node_widget.hpp"
#include "hesiod/gui/widgets/select_string_dialog.hpp"
#include "hesiod/gui/widgets/viewers/viewer_3d.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/graph/graph_node.hpp"
#include "hesiod/model/nodes/node_factory.hpp"
#include "hesiod/model/utils.hpp"

namespace hesiod
{

GraphNodeWidget::GraphNodeWidget(std::weak_ptr<GraphNode> p_graph_node, QWidget *parent)
    : GraphViewer("", parent), p_graph_node(p_graph_node)
{
  Logger::log()->trace("GraphNodeWidget::GraphNodeWidget: id: {}", this->get_id());

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  this->set_id(gno->get_id());
  this->setAttribute(Qt::WA_DeleteOnClose);

  // Undo / Redo stack
  this->undo_stack = new QUndoStack(this);
  this->undo_stack->setUndoLimit(50);

  // populate node catalog
  this->set_node_inventory(get_node_inventory());
  this->setup_connections();
}

GraphNodeWidget::~GraphNodeWidget()
{
  Logger::log()->trace("GraphNodeWidget::~GraphNodeWidget");

  // cancel any running background compute
  this->cancel_background_compute();

  // clean-up, viewer are not owned by this
  this->clear_data_viewers();
}

void GraphNodeWidget::add_import_texture_nodes(
    const std::vector<std::string> &texture_paths)
{
  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  QRectF bbox = this->get_bounding_box();

  AppContext &ctx = HSD_CTX;
  QPointF     delta = QPointF(
      ctx.app_settings.node_editor.position_delta_when_duplicating_node,
      ctx.app_settings.node_editor.position_delta_when_duplicating_node);

  float dy = 0.f;
  for (auto &fname : texture_paths)
  {
    Logger::log()->trace("GraphNodeWidget::add_import_texture_nodes: {}", fname);

    // create both model and graphic nodes
    std::string node_id = this->on_new_node_request("ImportTexture",
                                                    bbox.topRight() +
                                                        QPointF(delta.x(), dy));

    // setup attributes
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
    if (p_node)
    {
      // p_node->json_from(json_node["settings"]);
      // p_node->set_id(node_id);
      auto *p_attr = p_node->get_attributes_ref()
                         ->at("fname")
                         ->get_ref<attr::FilenameAttribute>();
      p_attr->set_value(fname);

      p_node->compute();
    }
    else
    {
      Logger::log()->error(
          "GraphNodeWidget::add_import_texture_nodes: dangling ptr for node_id {}",
          node_id);
    }

    dy += delta.y();
  }
}

void GraphNodeWidget::automatic_node_layout()
{
  Logger::log()->trace("GraphNodeWidget::automatic_node_layout");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  std::vector<gnode::Point> points = gno->compute_graph_layout_sugiyama();

  AppContext &ctx = HSD_CTX;
  QPointF     delta = QPointF(ctx.app_settings.node_editor.auto_layout_dx,
                          ctx.app_settings.node_editor.auto_layout_dy);
  QRectF      bbox = this->get_bounding_box();
  QPointF     origin = bbox.topLeft();

  size_t k = 0;

  for (auto &[nid, _] : gno->get_nodes())
  {
    if (k > points.size() - 1)
    {
      Logger::log()->error(
          "GraphNodeWidget::automatic_node_layout: computed layout is incoherent "
          "with current graphics node layout");
      return;
    }

    QPointF scene_pos = origin +
                        QPointF(points[k].x * delta.x(), points[k].y * delta.y());
    k++;

    gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(nid);

    if (p_gfx_node)
      p_gfx_node->setPos(scene_pos);
  }

  QTimer::singleShot(0, [this]() { this->zoom_to_content(); });
}

void GraphNodeWidget::backup_selected_ids()
{
  this->selected_ids = this->get_selected_node_ids();
}

void GraphNodeWidget::clear_all()
{
  this->clear_graphic_scene();

  Q_EMIT this->has_been_cleared(this->get_id());
}

void GraphNodeWidget::clear_data_viewers()
{
  for (auto viewer : this->data_viewers)
  {
    if (viewer)
      if (Viewer *p_viewer = dynamic_cast<Viewer *>(viewer.get()))
      {
        p_viewer->clear();
        p_viewer->deleteLater();
      }
  }
  this->data_viewers.clear();
}

void GraphNodeWidget::clear_graphic_scene()
{
  this->set_enabled(false);
  this->clear_data_viewers();
  GraphViewer::clear();
  this->set_enabled(true);
}

void GraphNodeWidget::closeEvent(QCloseEvent *event)
{
  this->clear_data_viewers();
  gngui::GraphViewer::closeEvent(event);
}

QScrollArea *GraphNodeWidget::create_attributes_scroll(QWidget *parent, QWidget *widget)
{
  auto *scroll = new QScrollArea(parent);
  scroll->setWidget(widget);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  int max_height = widget->sizeHint().height();

  if (QScreen *screen = this->screen())
  {
    int screen_h = screen->size().height();
    max_height = std::min(max_height, int(0.9 * screen_h));
  }

  scroll->setMinimumHeight(max_height);
  scroll->setMinimumWidth(widget->width() + 16);

  return scroll;
}

bool GraphNodeWidget::get_is_selecting_with_rubber_band() const
{
  return this->is_selecting_with_rubber_band;
}

GraphNode *GraphNodeWidget::get_p_graph_node()
{
  auto gno = this->p_graph_node.lock();
  if (!gno)
  {
    Logger::log()->critical(
        "GraphNodeWidget::get_p_graph_node: model graph_node reference is a "
        "dangling ptr");
    throw std::runtime_error("dangling ptr");
  }

  return gno.get();
}

void GraphNodeWidget::json_from(nlohmann::json const &json)
{
  Logger::log()->trace("GraphNodeWidget::json_from");
  this->clear_graphic_scene();

  // to prevent nodes update at each link creation when the loading
  // the graph (very slooow)
  this->update_node_on_connection_finished = false;
  this->suppress_undo_ = true;
  GraphViewer::json_from(json);
  this->suppress_undo_ = false;
  this->update_node_on_connection_finished = true;

  // viewers
  if (json.contains("viewers") && json["viewers"].is_array())
  {
    for (const auto &viewer_json : json["viewers"])
    {
      ViewerType viewer_type = viewer_json["viewer_type"].get<ViewerType>();

      Logger::log()->trace("GraphNodeWidget::json_from: viewer_type: {}",
                           viewer_type_as_string.at(viewer_type));

      // TODO add viewer-type specific handling
      this->on_viewport_request();

      if (auto *p_viewer = dynamic_cast<Viewer3D *>(this->data_viewers.back().get()))
      {
        // defer to let OpenGL context settle
        QTimer::singleShot(0,
                           [p_viewer, viewer_json]()
                           { p_viewer->json_from(viewer_json); });
      }
      else
        Logger::log()->error(
            "GraphNodeWidget::json_from: could not retrieve viewer reference");
    }
  }

  // defer
  QTimer::singleShot(0,
                     [this]()
                     {
                       this->update();
                       this->zoom_to_content();
                     });
}

nlohmann::json GraphNodeWidget::json_import(nlohmann::json const &json, QPointF scene_pos)
{
  // import used when copy/pasting only
  Logger::log()->trace("GraphNodeWidget::json_import");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return nlohmann::json();

  // Suppress individual undo commands during import (the caller handles undo)
  this->suppress_undo_ = true;

  // work on a copy of the json to modify the node IDs and return it
  nlohmann::json json_copy = json;

  // nodes
  if (!json_copy["nodes"].is_null())
  {
    // storage of id correspondance storage between original node and it copied version
    std::map<std::string, std::string> copy_id_map = {};

    for (auto &json_node : json_copy["nodes"])
    {
      QPointF pos = scene_pos;
      QPointF delta = QPointF(json_node["scene_position.x"],
                              json_node["scene_position.y"]);

      // create both model and graphic nodes
      std::string node_id = this->on_new_node_request(json_node["caption"], pos + delta);

      // use this new node id and backup it for links
      copy_id_map[json_node["id"].get<std::string>()] = node_id;
      json_node["id"] = node_id;

      // setup attributes
      BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
      p_node->json_from(json_node["settings"]);
      p_node->set_id(node_id);
    }

    // links
    if (!json_copy["links"].is_null())
      for (auto &json_link : json_copy["links"])
      {
        std::string node_id_from = json_link["node_out_id"].get<std::string>();
        std::string node_id_to = json_link["node_in_id"].get<std::string>();

        node_id_from = copy_id_map.at(node_id_from);
        node_id_to = copy_id_map.at(node_id_to);

        std::string port_out_id = json_link["port_out_id"];
        std::string port_in_id = json_link["port_in_id"];

        // add graphics link
        this->add_link(node_id_from, port_out_id, node_id_to, port_in_id);

        // add model link
        gno->new_link(node_id_from, port_out_id, node_id_to, port_in_id);
      }

    // background compute: mark all dirty, sort, dispatch
    {
      std::vector<std::string> dirty_ids;
      for (auto &[nid, _] : gno->get_nodes())
      {
        gno->get_node_ref_by_id(nid)->is_dirty = true;
        dirty_ids.push_back(nid);
      }
      auto sorted = gno->topological_sort(dirty_ids);
      this->start_background_compute(sorted);
    }
  }

  this->suppress_undo_ = false;

  return json_copy;
}

nlohmann::json GraphNodeWidget::json_to() const
{
  Logger::log()->trace("GraphNodeWidget::json_to");
  nlohmann::json json = GraphViewer::json_to();

  // add the viewports
  for (auto widget : this->data_viewers)
    if (widget)
      if (auto *p_viewer = dynamic_cast<Viewer *>(widget.get()))
      {
        json["viewers"].push_back(p_viewer->json_to());
      }

  return json;
}

void GraphNodeWidget::on_connection_deleted(const std::string &id_out,
                                            const std::string &port_id_out,
                                            const std::string &id_in,
                                            const std::string &port_id_in,
                                            bool               prevent_graph_update)
{
  if (this->is_computing_)
    return;

  Logger::log()->trace("GraphNodeWidget::on_connection_deleted, {}/{} -> {}/{}",
                       id_out,
                       port_id_out,
                       id_in,
                       port_id_in);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  this->set_enabled(false);

  // see GraphNodeWidget::on_node_deleted
  QCoreApplication::processEvents();

  gno->remove_link(id_out, port_id_out, id_in, port_id_in);

  // Push undo command for user-initiated link removal (skip during node deletion
  // and during undo/redo replay)
  if (!this->suppress_undo_ && !prevent_graph_update && this->undo_stack)
    this->undo_stack->push(
        new RemoveLinkCommand(this, id_out, port_id_out, id_in, port_id_in));

  // see GraphNodeWidget::on_node_deleted
  QCoreApplication::processEvents();

  // Only trigger graph update if the destination node still exists in the graph
  // (it may have been removed during a batch-delete operation)
  if (!prevent_graph_update && !gno->is_node_id_available(id_in))
  {
    auto sorted = gno->get_nodes_to_update(id_in);
    this->start_background_compute(sorted);
  }
  else
  {
    this->set_enabled(true);
  }
}

void GraphNodeWidget::on_connection_dropped(const std::string &node_id,
                                            const std::string &port_id,
                                            QPointF /*scene_pos*/)
{
  Logger::log()->trace("GraphNodeWidget::on_connection_dropped: {}/{}", node_id, port_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  bool ret = this->execute_new_node_context_menu();

  // if a node has indeed being created, arbitrarily connect the first
  // output with the same type has the output from which the link has
  // been emitted
  if (ret)
  {
    Logger::log()->trace("GraphNodeWidget::on_connection_dropped: auto-connecting nodes");

    const std::string node_to = this->last_node_created_id;

    BaseNode *p_node_from = gno->get_node_ref_by_id<BaseNode>(node_id);
    BaseNode *p_node_to = gno->get_node_ref_by_id<BaseNode>(node_to);

    if (p_node_to)
    {
      int         from_port_index = p_node_from->get_port_index(port_id);
      std::string from_type = p_node_from->get_data_type(from_port_index);

      Logger::log()->trace("GraphNodeWidget::on_connection_dropped: from_type: {}",
                           from_type);

      // connect to the first input that has the same type, if any
      for (int k = 0; k < p_node_to->get_nports(); ++k)
      {
        std::string to_type = p_node_to->get_data_type(k);

        if (to_type == from_type)
        {
          std::string port_to_id = p_node_to->get_port_label(k);
          this->add_link(node_id, port_id, node_to, port_to_id);
          break;
        }
      }
    }
    else
    {
      Logger::log()->trace(
          "GraphNodeWidget::on_connection_dropped: p_node_to is nullptr");
    }
  }
}

void GraphNodeWidget::on_connection_finished(const std::string &id_out,
                                             const std::string &port_id_out,
                                             const std::string &id_in,
                                             const std::string &port_id_in)
{
  if (this->is_computing_)
    return;

  Logger::log()->trace("GraphNodeWidget::on_connection_finished, {}/{} -> {}/{}",
                       id_out,
                       port_id_out,
                       id_in,
                       port_id_in);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  gno->new_link(id_out, port_id_out, id_in, port_id_in);

  // Push undo command for user-initiated link creation
  if (!this->suppress_undo_ && this->undo_stack &&
      this->update_node_on_connection_finished)
    this->undo_stack->push(
        new AddLinkCommand(this, id_out, port_id_out, id_in, port_id_in));

  // no update for instance during deserialization to avoid a full
  // graph update at each link creation
  if (this->update_node_on_connection_finished)
  {
    auto sorted = gno->get_nodes_to_update(id_in);
    this->start_background_compute(sorted);
  }
}

void GraphNodeWidget::on_graph_clear_request()
{
  if (this->is_computing_)
    return;

  Logger::log()->trace("GraphNodeWidget::on_graph_clear_request");

  QMessageBox::StandardButton reply = QMessageBox::question(
      nullptr,
      "?",
      "This will clear everything. Are you sure?",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes)
    this->clear_all();
}

void GraphNodeWidget::on_graph_import_request()
{
  Logger::log()->trace("GraphNodeWidget::on_graph_import_request");

  // define import path for dialog fiel
  std::filesystem::path path = this->last_import_path.empty()
                                   ? HSD_CTX.project_model->get_path()
                                   : this->last_import_path;

  this->set_enabled(false);
  QString load_fname = QFileDialog::getOpenFileName(this,
                                                    "Load...",
                                                    path.string().c_str(),
                                                    "Hesiod files (*.hsd)");
  this->set_enabled(true);

  if (!load_fname.isNull() && !load_fname.isEmpty())
  {
    std::string    fname = load_fname.toStdString();
    nlohmann::json json = json_from_file(fname);

    // retrieve the number of graph
    size_t n_graph = json["graph_manager"]["graph_nodes"].size();

    std::vector<std::string> graph_id_list = {};
    for (auto [key, _] : json["graph_manager"]["graph_nodes"].items())
      graph_id_list.push_back(key);

    if (n_graph < 1)
    {
      Logger::log()->warn("GraphNodeWidget::on_graph_import_request: no graph to import");
      return;
    }

    std::string import_graph_id;

    if (n_graph == 1)
    {
      import_graph_id = graph_id_list.back();
    }
    else
    {
      SelectStringDialog dialog(graph_id_list, "Select the graph layer to import:");

      if (dialog.exec() != QDialog::Accepted)
      {
        Logger::log()->trace("GraphNodeWidget::on_graph_import_request: aborted");
        return;
      }

      import_graph_id = dialog.selected_value();
    }

    Logger::log()->trace(
        "GraphNodeWidget::on_graph_import_request: importing {} from file {}",
        import_graph_id,
        fname);

    // build a json file that can be imported, see GraphNodeWidget::json_import
    nlohmann::json json_imp;
    json_imp = json["graph_tabs_widget"]["graph_node_widgets"][import_graph_id];

    // add node settings
    for (auto &json_node : json_imp["nodes"])
    {
      const std::string node_id = json_node["id"];

      // retrieve in the model section the corresponding node and
      // its parameters
      for (auto &json_node_model :
           json["graph_manager"]["graph_nodes"][import_graph_id]["nodes"])
      {
        if (json_node_model["id"] == node_id)
          json_node["settings"] = json_node_model;
      }
    }

    nlohmann::json json_mod = this->json_import(json_imp);

    // set selection on copied nodes
    this->deselect_all();

    for (auto &json_node : json_mod["nodes"])
    {
      const std::string    node_id = json_node["id"].get<std::string>();
      gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(node_id);

      // Qt mystery, this needs to be delayed to be effective
      QTimer::singleShot(0,
                         [p_gfx_node]()
                         {
                           if (p_gfx_node)
                             p_gfx_node->setSelected(true);
                         });
    }

    this->last_import_path = std::filesystem::path(fname).parent_path();
  }
}

void GraphNodeWidget::on_graph_new_request()
{
  Logger::log()->trace("GraphNodeWidget::on_graph_new_request");
  this->clear_all();
}

void GraphNodeWidget::on_graph_reload_request()
{
  Logger::log()->trace("GraphNodeWidget::on_graph_reload_request");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // full update: mark all dirty, sort, dispatch
  std::vector<std::string> dirty_ids;
  for (auto &[nid, _] : gno->get_nodes())
  {
    gno->get_node_ref_by_id(nid)->is_dirty = true;
    dirty_ids.push_back(nid);
  }
  auto sorted = gno->topological_sort(dirty_ids);
  this->start_background_compute(sorted);
}

void GraphNodeWidget::on_graph_settings_request()
{
  if (this->is_computing_)
    return;

  Logger::log()->trace("GraphNodeWidget::on_graph_settings_request");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // work on a copy of the model configuration before
  // apllying modifications
  GraphConfig       new_config = *gno->get_config_ref();
  GraphConfigDialog model_config_editor(new_config);

  int ret = model_config_editor.exec();

  if (ret)
  {
    this->backup_selected_ids();

    this->set_enabled(false);
    gno->change_config_values(new_config);
    this->set_enabled(true);

    this->reselect_backup_ids();
  }
}

void GraphNodeWidget::on_new_graphics_node_request(const std::string &node_id,
                                                   QPointF            scene_pos)
{
  // GraphicsNodes cannot generated by the GraphViewer instance by
  // itself, it is outsourced to the outer nodes manager (this
  // class). This slot respond to a request for the creation of a
  // GraphicsNodes (only). This is different from
  // GraphNodeWidget::on_new_node_request which generates both the
  // model and the GUI nodes...

  // This one is actually used for serialization, when the graph
  // viewer requests the creation of a graphics node while the base
  // node has aldready been created when the GraphNode has been
  // deserialized

  Logger::log()->trace("GraphNodeWidget::on_new_graphics_node_request: {} {},{}",
                       node_id,
                       scene_pos.x(),
                       scene_pos.y());

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
  auto     *p_proxy = new gngui::TypedNodeProxy<BaseNode>(p_node->get_shared());
  auto *widget = node_widget_factory(p_node->get_caption(), p_node->get_shared(), this);

  this->add_node(p_proxy, scene_pos, node_id);
  this->get_graphics_node_by_id(node_id)->set_widget(widget);
}

std::string GraphNodeWidget::on_new_node_request(const std::string &node_type,
                                                 QPointF            scene_pos)
{
  if (this->is_computing_)
    return "";

  Logger::log()->trace("GraphNodeWidget::on_new_node_request: node_type {}", node_type);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return "";

  if (node_type == "")
    return "";

  // add control node (compute)
  std::string node_id = gno->add_node(node_type);

  // add corresponding graphics node (GUI)
  this->on_new_graphics_node_request(node_id, scene_pos);

  Q_EMIT this->new_node_created(this->get_id(), node_id);

  this->last_node_created_id = node_id;

  // Push undo command (skip during undo/redo replay and import operations)
  if (!this->suppress_undo_ && this->undo_stack)
    this->undo_stack->push(new AddNodeCommand(this, node_id, node_type, scene_pos));

  return node_id;
}

void GraphNodeWidget::on_node_dropped_on_link(const std::string &dropped_node_id,
                                              const std::string &link_out_id,
                                              const std::string &link_out_port_id,
                                              const std::string &link_in_id,
                                              const std::string &link_in_port_id)
{
  Logger::log()->trace("GraphNodeWidget::on_node_dropped_on_link: {} onto {}:{} -> {}:{}",
                       dropped_node_id,
                       link_out_id,
                       link_out_port_id,
                       link_in_id,
                       link_in_port_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Get the dropped node
  BaseNode *p_dropped = gno->get_node_ref_by_id<BaseNode>(dropped_node_id);
  if (!p_dropped)
    return;

  // Find the data type of the link's output port
  BaseNode *p_link_out = gno->get_node_ref_by_id<BaseNode>(link_out_id);
  BaseNode *p_link_in = gno->get_node_ref_by_id<BaseNode>(link_in_id);
  if (!p_link_out || !p_link_in)
    return;

  int         link_out_port_idx = p_link_out->get_port_index(link_out_port_id);
  int         link_in_port_idx = p_link_in->get_port_index(link_in_port_id);
  std::string data_type_out = p_link_out->get_data_type(link_out_port_idx);
  std::string data_type_in = p_link_in->get_data_type(link_in_port_idx);

  // Find compatible input port on dropped node (matches the link source type)
  std::string dropped_in_port_id;
  for (int k = 0; k < p_dropped->get_nports(); ++k)
  {
    if (p_dropped->get_port_type(k) == gngui::PortType::IN &&
        p_dropped->get_data_type(k) == data_type_out)
    {
      dropped_in_port_id = p_dropped->get_port_label(k);
      break;
    }
  }

  // Find compatible output port on dropped node (matches the link target type)
  std::string dropped_out_port_id;
  for (int k = 0; k < p_dropped->get_nports(); ++k)
  {
    if (p_dropped->get_port_type(k) == gngui::PortType::OUT &&
        p_dropped->get_data_type(k) == data_type_in)
    {
      dropped_out_port_id = p_dropped->get_port_label(k);
      break;
    }
  }

  if (dropped_in_port_id.empty() || dropped_out_port_id.empty())
  {
    Logger::log()->trace("GraphNodeWidget::on_node_dropped_on_link: no compatible ports");
    return;
  }

  // 1. Delete the original link (both graphics and model)
  gngui::GraphicsLink *p_gfx_link = nullptr;
  for (QGraphicsItem *item : this->scene()->items())
  {
    if (auto *link = dynamic_cast<gngui::GraphicsLink *>(item))
    {
      if (link->get_node_out() && link->get_node_in() &&
          link->get_node_out()->get_id() == link_out_id &&
          link->get_node_in()->get_id() == link_in_id &&
          link->get_node_out()->get_port_id(link->get_port_out_index()) ==
              link_out_port_id &&
          link->get_node_in()->get_port_id(link->get_port_in_index()) == link_in_port_id)
      {
        p_gfx_link = link;
        break;
      }
    }
  }

  if (p_gfx_link)
  {
    bool link_will_be_replaced = true;
    this->delete_graphics_link(p_gfx_link, link_will_be_replaced);
  }

  gno->remove_link(link_out_id, link_out_port_id, link_in_id, link_in_port_id);

  // 2. Create link: original_source -> dropped_node_input
  this->add_link(link_out_id, link_out_port_id, dropped_node_id, dropped_in_port_id);
  gno->new_link(link_out_id, link_out_port_id, dropped_node_id, dropped_in_port_id);

  // 3. Create link: dropped_node_output -> original_target
  this->add_link(dropped_node_id, dropped_out_port_id, link_in_id, link_in_port_id);
  gno->new_link(dropped_node_id, dropped_out_port_id, link_in_id, link_in_port_id);

  // 4. Trigger graph update from the dropped node
  {
    auto sorted = gno->get_nodes_to_update(dropped_node_id);
    this->start_background_compute(sorted);
  }
}

void GraphNodeWidget::on_node_deleted_request(const std::string &node_id)
{
  if (this->is_computing_)
    return;

  Logger::log()->trace("GraphNodeWidget::on_node_deleted_request, node {}", node_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Guard: only remove if the node still exists in the graph model
  // (it may have already been removed by a prior operation in the same batch)
  if (gno->is_node_id_available(node_id))
  {
    Logger::log()->warn("GraphNodeWidget::on_node_deleted_request: node {} "
                        "already removed from graph",
                        node_id);
    return;
  }

  this->set_enabled(false);
  gno->remove_node(node_id);
  this->set_enabled(true);

  Q_EMIT this->node_deleted(this->get_id(), node_id);
}

void GraphNodeWidget::on_node_info(const std::string &node_id)
{
  Logger::log()->trace("GraphNodeWidget::on_node_info, node {}", node_id);

  if (!node_id.empty())
  {
    NodeInfoDialog *dialog = new NodeInfoDialog(this, node_id, this);
    dialog->show();
  }
}

void GraphNodeWidget::on_node_pinned(const std::string &node_id, bool state)
{
  Logger::log()->trace("GraphNodeWidget::on_node_pinned, node {}", node_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  this->unpin_nodes();

  if (!node_id.empty())
  {
    // TODO make a dedicated GraphViewer method
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
    if (!p_node)
      return;

    gngui::GraphicsNode *p_gx_node = this->get_graphics_node_by_id(node_id);
    if (!p_gx_node)
      return;

    p_gx_node->set_is_node_pinned(state);
  }
}

void GraphNodeWidget::on_node_reload_request(const std::string &node_id)
{
  Logger::log()->trace("GraphNodeWidget::on_node_reload_request, node [{}]", node_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  auto sorted = gno->get_nodes_to_update(node_id);
  this->start_background_compute(sorted);
}

void GraphNodeWidget::on_node_right_clicked(const std::string &node_id, QPointF scene_pos)
{
  Logger::log()->trace("GraphNodeWidget::on_node_right_clicked: id {}", node_id);

  // only show custom menu if clicked inside the top area of the node,
  // outside the embedded widget
  {
    gngui::GraphicsNode *p_gx_node = this->get_graphics_node_by_id(node_id);
    if (!p_gx_node)
      return;

    QPointF item_pos = scene_pos - p_gx_node->scenePos();
    if (item_pos.y() >= p_gx_node->get_geometry().widget_pos.y())
      return;
  }

  // settings widget
  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  QWidget *attr_widget = new NodeAttributesWidget(gno->get_shared(),
                                                  node_id,
                                                  /* p_graph_node_widget */ this,
                                                  /* add_toolbar */ true,
                                                  /* parent */ this);
  if (!attr_widget)
    return;

  // create menu
  CustomQMenu *menu = new CustomQMenu();

  {
    auto *scroll = this->create_attributes_scroll(menu, attr_widget);
    auto *scroll_action = new QWidgetAction(menu);
    scroll_action->setDefaultWidget(scroll);
    menu->addAction(scroll_action);
  }

  menu->setWindowFlags(menu->windowFlags() | Qt::Popup);
  menu->setAttribute(Qt::WA_NoMousePropagation, false);
  menu->popup(QCursor::pos());
}

void GraphNodeWidget::on_nodes_copy_request(const std::vector<std::string> &id_list,
                                            const std::vector<QPointF> &scene_pos_list)
{
  Logger::log()->trace("GraphNodeWidget::on_nodes_copy_request");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // dump the nodes data into the copy buffer (for the node positions,
  // save the node position relative to the mouse cursor)
  this->json_copy_buffer.clear();

  QPoint  mouse_view_pos = this->mapFromGlobal(QCursor::pos());
  QPointF mouse_scene_pos = this->mapToScene(mouse_view_pos);

  // dump to a json with a structure that can be read by
  // GraphViewer::json_from

  json_copy_buffer["nodes"] = nlohmann::json::array();

  for (size_t k = 0; k < id_list.size(); k++)
  {
    nlohmann::json json_node;

    gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(id_list[k]);
    json_node = p_gfx_node->json_to();

    // override absolute positions with relative positions
    QPointF delta = scene_pos_list[k] - mouse_scene_pos;
    json_node["scene_position.x"] = delta.x();
    json_node["scene_position.y"] = delta.y();

    // add attribute settings to set them back when pasting (not
    // required by GraphViewer::json_from)
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(id_list[k]);
    json_node["settings"] = p_node->json_to();

    this->json_copy_buffer["nodes"].push_back(json_node);
  }

  // backup links between copied nodes
  this->json_copy_buffer["links"] = nlohmann::json::array();

  for (auto &link : gno->get_links())
  {
    // only keep a link if both nodes are in copy buffer
    if (contains(id_list, link.from) && contains(id_list, link.to))
    {
      nlohmann::json json_link;

      gnode::Node *p_from = gno->get_node_ref_by_id(link.from);
      gnode::Node *p_to = gno->get_node_ref_by_id(link.to);

      json_link["node_out_id"] = link.from;
      json_link["node_in_id"] = link.to;
      json_link["port_out_id"] = p_from->get_port_label(link.port_from);
      json_link["port_in_id"] = p_to->get_port_label(link.port_to);

      this->json_copy_buffer["links"].push_back(json_link);
    }
  }

  Q_EMIT this->copy_buffer_has_changed(this->json_copy_buffer);
}

void GraphNodeWidget::on_nodes_duplicate_request(
    const std::vector<std::string> &id_list,
    const std::vector<QPointF>     &scene_pos_list)
{
  Logger::log()->trace("GraphNodeWidget::on_nodes_duplicate_request");

  std::vector<QPointF> scene_pos_shifted = {};

  AppContext &ctx = HSD_CTX;
  QPointF     delta = QPointF(
      ctx.app_settings.node_editor.position_delta_when_duplicating_node,
      ctx.app_settings.node_editor.position_delta_when_duplicating_node);

  for (auto &p : scene_pos_list)
    scene_pos_shifted.push_back(p + delta);

  this->on_nodes_copy_request(id_list, scene_pos_shifted);
  this->on_nodes_paste_request();
}

void GraphNodeWidget::on_nodes_paste_request()
{
  Logger::log()->trace("GraphNodeWidget::on_nodes_paste_request");

  if (!this->json_copy_buffer.is_object())
    return;

  QPointF mouse_scene_pos = this->get_mouse_scene_pos();

  // returned json contains modified node IDs
  nlohmann::json json_mod = this->json_import(this->json_copy_buffer, mouse_scene_pos);

  // set selection on copied nodes
  this->deselect_all();

  for (auto &json_node : json_mod["nodes"])
  {
    const std::string    node_id = json_node["id"].get<std::string>();
    gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(node_id);
    if (p_gfx_node)
      p_gfx_node->setSelected(true);
  }
}

void GraphNodeWidget::on_viewport_request()
{
  Logger::log()->trace("GraphNodeWidget::on_viewport_request");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  for (auto &[id, _] : gno->get_nodes())
  {
    gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(id);
    if (!p_gfx)
    {
      Logger::log()->critical("GraphNodeWidget::on_graph_settings_request: GraphicsNode "
                              "ref is nullptr for id {}",
                              id);
    }
  }

  this->data_viewers.push_back(new Viewer3D(this)); // no parent, independant window
  Logger::log()->trace("ok0");
  this->data_viewers.back()->show();
  Logger::log()->trace("ok1");
  Viewer *p_viewer = dynamic_cast<Viewer *>(this->data_viewers.back().get());
  Logger::log()->trace("ok2");

  // remove the widget from the widget list if it is closed
  this->connect(p_viewer,
                &Viewer::widget_close,
                [this, p_viewer]()
                {
                  std::erase_if(this->data_viewers,
                                [p_viewer](QWidget *ptr) { return ptr == p_viewer; });
                });
  Logger::log()->trace("ok3");

  // set data of the currently selected node, if any
  std::vector<std::string> selected_ids = this->get_selected_node_ids();

  if (selected_ids.size())
    p_viewer->on_node_selected(selected_ids.back());
}

void GraphNodeWidget::reselect_backup_ids()
{
  QTimer::singleShot(
      0,
      [this]()
      {
        for (size_t k = 0; k < this->selected_ids.size(); ++k)
        {
          const std::string nid = this->selected_ids[this->selected_ids.size() - 1 - k];
          if (gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(nid))
            p_gfx_node->setSelected(true);
        }
        this->selected_ids.clear();
      });
}

void GraphNodeWidget::keyPressEvent(QKeyEvent *event)
{
  // Cancel background compute on Escape
  if (event->key() == Qt::Key_Escape && this->is_computing_)
  {
    this->cancel_background_compute();
    event->accept();
    return;
  }

  // Intercept Delete/Backspace to go through the undo stack
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
  {
    this->delete_selected_with_undo();
    event->accept();
    return;
  }

  // All other keys handled by GraphViewer
  GraphViewer::keyPressEvent(event);
}

void GraphNodeWidget::mousePressEvent(QMouseEvent *event)
{
  // Capture positions of selected nodes before drag starts (for move undo)
  if (event->button() == Qt::LeftButton)
  {
    this->drag_start_positions_.clear();
    std::vector<QPointF>     pos_list;
    std::vector<std::string> ids = this->get_selected_node_ids(&pos_list);
    for (size_t k = 0; k < ids.size(); k++)
      this->drag_start_positions_[ids[k]] = pos_list[k];
  }

  GraphViewer::mousePressEvent(event);
}

void GraphNodeWidget::mouseReleaseEvent(QMouseEvent *event)
{
  GraphViewer::mouseReleaseEvent(event);

  // Check if selected nodes actually moved and push a MoveNodesCommand
  if (event->button() == Qt::LeftButton && !this->drag_start_positions_.empty())
  {
    std::map<std::string, QPointF> new_positions;
    bool any_moved = false;

    for (auto &[node_id, old_pos] : this->drag_start_positions_)
    {
      gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(node_id);
      if (!p_gfx)
        continue;

      QPointF new_pos = p_gfx->scenePos();
      new_positions[node_id] = new_pos;

      // Check for meaningful movement (more than 0.5px to avoid float noise)
      QPointF diff = new_pos - old_pos;
      if (qAbs(diff.x()) > 0.5 || qAbs(diff.y()) > 0.5)
        any_moved = true;
    }

    if (any_moved && this->undo_stack)
      this->undo_stack->push(
          new MoveNodesCommand(this, this->drag_start_positions_, new_positions));

    this->drag_start_positions_.clear();
  }
}

void GraphNodeWidget::delete_selected_with_undo()
{
  Logger::log()->trace("GraphNodeWidget::delete_selected_with_undo");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Gather selected node IDs + positions
  std::vector<QPointF>     scene_pos_list;
  std::vector<std::string> id_list = this->get_selected_node_ids(&scene_pos_list);

  if (id_list.empty())
  {
    // No nodes selected — fall through to the base class to handle
    // selected links or other items
    this->delete_selected_items();
    return;
  }

  // Serialize selected nodes into a JSON snapshot
  nlohmann::json snapshot;
  snapshot["nodes"] = nlohmann::json::array();

  for (size_t k = 0; k < id_list.size(); k++)
  {
    nlohmann::json json_node;

    gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(id_list[k]);
    if (!p_gfx_node)
      continue;

    json_node = p_gfx_node->json_to();

    // Store absolute positions (undo restores to original location)
    json_node["scene_position.x"] = scene_pos_list[k].x();
    json_node["scene_position.y"] = scene_pos_list[k].y();

    // Store node attribute settings for full restore
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(id_list[k]);
    if (p_node)
      json_node["settings"] = p_node->json_to();

    snapshot["nodes"].push_back(json_node);
  }

  // Capture ALL links touching the selected nodes: internal AND external
  snapshot["internal_links"] = nlohmann::json::array();
  snapshot["external_links"] = nlohmann::json::array();

  for (auto &link : gno->get_links())
  {
    bool from_selected = contains(id_list, link.from);
    bool to_selected = contains(id_list, link.to);

    if (!from_selected && !to_selected)
      continue;

    gnode::Node *p_from = gno->get_node_ref_by_id(link.from);
    gnode::Node *p_to = gno->get_node_ref_by_id(link.to);
    if (!p_from || !p_to)
      continue;

    nlohmann::json json_link;
    json_link["node_out_id"] = link.from;
    json_link["node_in_id"] = link.to;
    json_link["port_out_id"] = p_from->get_port_label(link.port_from);
    json_link["port_in_id"] = p_to->get_port_label(link.port_to);

    if (from_selected && to_selected)
      snapshot["internal_links"].push_back(json_link);
    else
      snapshot["external_links"].push_back(json_link);
  }

  // Now perform the actual deletion (suppress undo to avoid spurious link removal commands)
  this->suppress_undo_ = true;
  this->delete_selected_items();
  this->suppress_undo_ = false;

  // Push the undo command (first_redo is skipped since we just deleted)
  if (this->undo_stack)
    this->undo_stack->push(new DeleteNodesCommand(this, snapshot, id_list));
}

void GraphNodeWidget::delete_nodes_by_ids(const std::vector<std::string> &ids)
{
  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Suppress undo pushes: deleting nodes triggers link removal signals which
  // would otherwise generate spurious RemoveLinkCommands
  bool was_suppressed = this->suppress_undo_;
  this->suppress_undo_ = true;

  for (auto &id : ids)
  {
    gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(id);
    if (p_gfx)
    {
      // delete_graphics_node emits node_deleted signal which triggers
      // on_node_deleted_request → gno->remove_node, so we must NOT call
      // remove_node again here
      this->delete_graphics_node(p_gfx);
    }
    else
    {
      // Graphics node already gone (e.g. redo after scene was rebuilt),
      // just remove from the model directly
      this->set_enabled(false);
      if (!gno->is_node_id_available(id))
        gno->remove_node(id);
      this->set_enabled(true);

      Q_EMIT this->node_deleted(this->get_id(), id);
    }
  }

  this->suppress_undo_ = was_suppressed;
}

void GraphNodeWidget::on_undo_request()
{
  Logger::log()->trace("GraphNodeWidget::on_undo_request");
  if (this->undo_stack && this->undo_stack->canUndo())
    this->undo_stack->undo();
}

void GraphNodeWidget::on_redo_request()
{
  Logger::log()->trace("GraphNodeWidget::on_redo_request");
  if (this->undo_stack && this->undo_stack->canRedo())
    this->undo_stack->redo();
}

void GraphNodeWidget::set_json_copy_buffer(nlohmann::json const &new_json_copy_buffer)
{
  this->json_copy_buffer = new_json_copy_buffer;
}

// =====================================
// Undo Command Helpers
// =====================================

void GraphNodeWidget::restore_nodes_from_snapshot(const nlohmann::json &snapshot)
{
  Logger::log()->trace("GraphNodeWidget::restore_nodes_from_snapshot");

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Suppress undo pushes during restore
  this->suppress_undo_ = true;

  // 1. Recreate nodes with their original IDs
  if (snapshot.contains("nodes"))
  {
    for (auto &json_node : snapshot["nodes"])
    {
      std::string caption = json_node.value("caption", "");
      std::string original_id = json_node.value("id", "");

      if (caption.empty() || original_id.empty())
        continue;

      QPointF pos(json_node.value("scene_position.x", 0.0f),
                  json_node.value("scene_position.y", 0.0f));

      // Create with original ID preserved
      this->create_node_with_id(caption, original_id, pos,
                                json_node.value("settings", nlohmann::json()));
    }
  }

  // 2. Restore internal links (between restored nodes)
  std::string internal_key = snapshot.contains("internal_links") ? "internal_links"
                                                                 : "links";
  if (snapshot.contains(internal_key))
  {
    for (auto &json_link : snapshot[internal_key])
    {
      std::string id_out = json_link.value("node_out_id", "");
      std::string id_in = json_link.value("node_in_id", "");
      std::string port_out = json_link.value("port_out_id", "");
      std::string port_in = json_link.value("port_in_id", "");

      if (id_out.empty() || id_in.empty())
        continue;

      // Both nodes are restored, safe to connect
      this->add_link(id_out, port_out, id_in, port_in);
      gno->new_link(id_out, port_out, id_in, port_in);
    }
  }

  // 3. Restore external links (connecting to surviving nodes)
  if (snapshot.contains("external_links"))
  {
    for (auto &json_link : snapshot["external_links"])
    {
      std::string id_out = json_link.value("node_out_id", "");
      std::string id_in = json_link.value("node_in_id", "");
      std::string port_out = json_link.value("port_out_id", "");
      std::string port_in = json_link.value("port_in_id", "");

      if (id_out.empty() || id_in.empty())
        continue;

      // Verify both endpoints exist before reconnecting (per reviewer note:
      // the surviving node might have been deleted by another operation)
      if (gno->is_node_id_available(id_out) || gno->is_node_id_available(id_in))
      {
        Logger::log()->warn(
            "GraphNodeWidget::restore_nodes_from_snapshot: skipping external link "
            "{}:{} -> {}:{} — endpoint no longer exists",
            id_out,
            port_out,
            id_in,
            port_in);
        continue;
      }

      // Verify the specific ports still exist on both nodes
      gnode::Node *p_from = gno->get_node_ref_by_id(id_out);
      gnode::Node *p_to = gno->get_node_ref_by_id(id_in);
      if (!p_from || !p_to)
        continue;

      int from_port_idx = p_from->get_port_index(port_out);
      int to_port_idx = p_to->get_port_index(port_in);
      if (from_port_idx < 0 || to_port_idx < 0)
      {
        Logger::log()->warn(
            "GraphNodeWidget::restore_nodes_from_snapshot: skipping external link "
            "— port not found: {}:{} -> {}:{}",
            id_out,
            port_out,
            id_in,
            port_in);
        continue;
      }

      this->add_link(id_out, port_out, id_in, port_in);
      gno->new_link(id_out, port_out, id_in, port_in);
    }
  }

  this->suppress_undo_ = false;

  // 4. Trigger recompute for restored nodes
  std::vector<std::string> dirty_ids;
  if (snapshot.contains("nodes"))
  {
    for (auto &json_node : snapshot["nodes"])
    {
      std::string nid = json_node.value("id", "");
      if (!nid.empty() && !gno->is_node_id_available(nid))
      {
        gno->get_node_ref_by_id(nid)->is_dirty = true;
        dirty_ids.push_back(nid);
      }
    }
  }
  if (!dirty_ids.empty())
  {
    auto sorted = gno->topological_sort(dirty_ids);
    this->start_background_compute(sorted);
  }
}

void GraphNodeWidget::create_node_with_id(const std::string    &node_type,
                                          const std::string    &node_id,
                                          QPointF               scene_pos,
                                          const nlohmann::json &settings)
{
  Logger::log()->trace("GraphNodeWidget::create_node_with_id: {} as {}", node_type, node_id);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Create the model node via the factory
  std::shared_ptr<gnode::Node> node = node_factory(node_type, gno->get_config_shared());
  node->compute();

  // Add with the specific ID (preserving the original)
  gno->add_node(node, node_id);

  // Restore settings if available
  if (!settings.is_null() && settings.is_object())
  {
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
    if (p_node)
    {
      p_node->json_from(settings);
      p_node->set_id(node_id);
    }
  }

  // Create the graphics node
  this->on_new_graphics_node_request(node_id, scene_pos);

  Q_EMIT this->new_node_created(this->get_id(), node_id);
}

void GraphNodeWidget::create_link_internal(const std::string &id_out,
                                           const std::string &port_id_out,
                                           const std::string &id_in,
                                           const std::string &port_id_in)
{
  Logger::log()->trace("GraphNodeWidget::create_link_internal: {}/{} -> {}/{}",
                       id_out,
                       port_id_out,
                       id_in,
                       port_id_in);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Verify endpoints exist
  if (gno->is_node_id_available(id_out) || gno->is_node_id_available(id_in))
  {
    Logger::log()->warn("GraphNodeWidget::create_link_internal: endpoint missing");
    return;
  }

  // Add graphics link
  this->add_link(id_out, port_id_out, id_in, port_id_in);

  // Add model link
  gno->new_link(id_out, port_id_out, id_in, port_id_in);

  // Trigger downstream compute
  auto sorted = gno->get_nodes_to_update(id_in);
  this->start_background_compute(sorted);
}

void GraphNodeWidget::remove_link_internal(const std::string &id_out,
                                           const std::string &port_id_out,
                                           const std::string &id_in,
                                           const std::string &port_id_in)
{
  Logger::log()->trace("GraphNodeWidget::remove_link_internal: {}/{} -> {}/{}",
                       id_out,
                       port_id_out,
                       id_in,
                       port_id_in);

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // Suppress undo pushes — this is called from undo/redo command replay
  this->suppress_undo_ = true;

  // Find and remove the graphics link. delete_graphics_link emits connection_deleted
  // which calls on_connection_deleted, which handles model removal and graph update.
  bool found = false;
  for (QGraphicsItem *item : this->scene()->items())
  {
    if (auto *link = dynamic_cast<gngui::GraphicsLink *>(item))
    {
      if (link->get_node_out() && link->get_node_in() &&
          link->get_node_out()->get_id() == id_out &&
          link->get_node_in()->get_id() == id_in &&
          link->get_node_out()->get_port_id(link->get_port_out_index()) == port_id_out &&
          link->get_node_in()->get_port_id(link->get_port_in_index()) == port_id_in)
      {
        this->delete_graphics_link(link, false);
        found = true;
        break;
      }
    }
  }

  // If no graphics link was found (edge case), remove model link directly
  if (!found)
  {
    gno->remove_link(id_out, port_id_out, id_in, port_id_in);

    if (!gno->is_node_id_available(id_in))
    {
      auto sorted = gno->get_nodes_to_update(id_in);
      this->start_background_compute(sorted);
    }
  }

  this->suppress_undo_ = false;
}

// =====================================
// Background Compute
// =====================================

bool GraphNodeWidget::is_computing() const { return this->is_computing_; }

void GraphNodeWidget::force_build()
{
  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  std::vector<std::string> dirty_ids;
  for (auto &[nid, _] : gno->get_nodes())
  {
    gno->get_node_ref_by_id(nid)->is_dirty = true;
    dirty_ids.push_back(nid);
  }
  auto sorted = gno->topological_sort(dirty_ids);
  this->start_background_compute(sorted);
}

void GraphNodeWidget::start_background_compute(const std::vector<std::string> &sorted_ids)
{
  if (sorted_ids.empty())
    return;

  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  if (this->is_computing_)
  {
    Logger::log()->warn("GraphNodeWidget::start_background_compute: already running");
    return;
  }

  this->is_computing_ = true;
  this->setEnabled(false);

  // Suppress GraphNode callbacks to avoid double-signaling from worker thread
  this->saved_compute_started_ = gno->compute_started;
  this->saved_compute_finished_ = gno->compute_finished;
  this->saved_update_started_ = gno->update_started;
  this->saved_update_finished_ = gno->update_finished;
  this->saved_update_progress_ = gno->update_progress;

  gno->compute_started = nullptr;
  gno->compute_finished = nullptr;
  gno->update_started = nullptr;
  gno->update_finished = nullptr;
  gno->update_progress = nullptr;

  Q_EMIT this->update_started();

  // Create worker and thread
  this->worker_thread_ = new QThread(this);
  this->graph_worker_ = new GraphWorker(); // no parent -- will be moved

  this->graph_worker_->configure(gno.get(), sorted_ids);
  this->graph_worker_->moveToThread(this->worker_thread_);

  // Connect worker signals to widget slots (auto queued cross-thread)
  connect(this->graph_worker_,
          &GraphWorker::node_compute_started,
          this,
          &GraphNodeWidget::on_worker_node_compute_started);
  connect(this->graph_worker_,
          &GraphWorker::node_compute_finished,
          this,
          &GraphNodeWidget::on_worker_node_compute_finished);
  connect(this->graph_worker_,
          &GraphWorker::progress_updated,
          this,
          &GraphNodeWidget::on_worker_progress_updated);
  connect(this->graph_worker_,
          &GraphWorker::node_execution_time,
          this,
          &GraphNodeWidget::on_worker_node_execution_time);
  connect(this->graph_worker_,
          &GraphWorker::compute_all_finished,
          this,
          &GraphNodeWidget::on_worker_compute_all_finished);

  // Start work when thread starts
  connect(this->worker_thread_,
          &QThread::started,
          this->graph_worker_,
          &GraphWorker::do_compute);

  // Clean up when done
  connect(this->graph_worker_,
          &GraphWorker::compute_all_finished,
          this->worker_thread_,
          &QThread::quit);
  connect(this->worker_thread_,
          &QThread::finished,
          this->graph_worker_,
          &QObject::deleteLater);
  connect(this->worker_thread_,
          &QThread::finished,
          this->worker_thread_,
          &QObject::deleteLater);

  this->worker_thread_->start();
}

void GraphNodeWidget::cancel_background_compute()
{
  if (this->graph_worker_)
    this->graph_worker_->request_cancel();

  if (this->worker_thread_ && this->worker_thread_->isRunning())
    this->worker_thread_->wait(5000);
}

void GraphNodeWidget::on_worker_node_compute_started(const std::string &node_id)
{
  gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(node_id);
  if (p_gfx)
    p_gfx->on_compute_started();

  Q_EMIT this->compute_started(node_id);
}

void GraphNodeWidget::on_worker_node_compute_finished(const std::string &node_id)
{
  gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(node_id);
  if (p_gfx)
  {
    p_gfx->on_compute_finished();

    if (HSD_CTX.app_settings.interface.enable_node_settings_in_node_body)
      p_gfx->update();
  }

  Q_EMIT this->compute_finished(node_id);
}

void GraphNodeWidget::on_worker_progress_updated(const std::string &node_id,
                                                 float              percent)
{
  gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(node_id);
  if (p_gfx)
    p_gfx->set_build_progress(static_cast<int>(percent));

  Q_EMIT this->update_progress(node_id, percent);
}

void GraphNodeWidget::on_worker_node_execution_time(const std::string &node_id,
                                                    float              time_ms,
                                                    int                backend_type)
{
  gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(node_id);
  if (p_gfx)
  {
    p_gfx->set_last_execution_time(time_ms);
    p_gfx->set_last_backend_type(backend_type);
    p_gfx->update(); // force immediate visual repaint of backend badge
  }
}

void GraphNodeWidget::on_worker_compute_all_finished(bool was_cancelled)
{
  // Restore suppressed callbacks
  auto gno = this->p_graph_node.lock();
  if (gno)
  {
    gno->compute_started = this->saved_compute_started_;
    gno->compute_finished = this->saved_compute_finished_;
    gno->update_started = this->saved_update_started_;
    gno->update_finished = this->saved_update_finished_;
    gno->update_progress = this->saved_update_progress_;
  }
  this->saved_compute_started_ = nullptr;
  this->saved_compute_finished_ = nullptr;
  this->saved_update_started_ = nullptr;
  this->saved_update_finished_ = nullptr;
  this->saved_update_progress_ = nullptr;

  this->is_computing_ = false;
  this->setEnabled(true);

  // Reset progress bars
  if (gno)
  {
    for (auto &[nid, _] : gno->get_nodes())
    {
      gngui::GraphicsNode *p_gfx = this->get_graphics_node_by_id(nid);
      if (p_gfx)
        p_gfx->set_build_progress(0);
    }
  }

  Q_EMIT this->update_finished();

  // Clean up references (objects will be deleted by deleteLater)
  this->worker_thread_ = nullptr;
  this->graph_worker_ = nullptr;

  // Call post_update on the graph
  if (gno)
    gno->post_update();

  if (was_cancelled)
    Logger::log()->info("GraphNodeWidget: compute was cancelled");
}

void GraphNodeWidget::setup_connections()
{
  auto gno = this->p_graph_node.lock();
  if (!gno)
    return;

  // global actions
  this->connect(this,
                &gngui::GraphViewer::graph_automatic_node_layout_request,
                this,
                &GraphNodeWidget::automatic_node_layout);

  this->connect(this,
                &gngui::GraphViewer::graph_clear_request,
                this,
                &GraphNodeWidget::on_graph_clear_request);

  this->connect(this,
                &gngui::GraphViewer::graph_import_request,
                this,
                &GraphNodeWidget::on_graph_import_request);

  this->connect(this,
                &gngui::GraphViewer::graph_new_request,
                this,
                &GraphNodeWidget::on_graph_new_request);

  this->connect(this,
                &gngui::GraphViewer::graph_reload_request,
                this,
                &GraphNodeWidget::on_graph_reload_request);

  this->connect(this,
                &gngui::GraphViewer::graph_settings_request,
                this,
                &GraphNodeWidget::on_graph_settings_request);

  this->connect(this,
                &gngui::GraphViewer::rubber_band_selection_started,
                [this]() { this->is_selecting_with_rubber_band = true; });

  this->connect(this,
                &gngui::GraphViewer::rubber_band_selection_finished,
                [this]() { this->is_selecting_with_rubber_band = false; });

  // GraphViewer -> GraphNodeWidget
  this->connect(this,
                &gngui::GraphViewer::connection_deleted,
                this,
                &GraphNodeWidget::on_connection_deleted);

  this->connect(this,
                &gngui::GraphViewer::connection_dropped,
                this,
                &GraphNodeWidget::on_connection_dropped);

  this->connect(this,
                &gngui::GraphViewer::connection_finished,
                this,
                &GraphNodeWidget::on_connection_finished);

  this->connect(this,
                &gngui::GraphViewer::new_graphics_node_request,
                this,
                &GraphNodeWidget::on_new_graphics_node_request);

  this->connect(this,
                &gngui::GraphViewer::new_node_request,
                this,
                &GraphNodeWidget::on_new_node_request);

  this->connect(this,
                &gngui::GraphViewer::node_deleted,
                this,
                &GraphNodeWidget::on_node_deleted_request);

  this->connect(this,
                &gngui::GraphViewer::node_reload_request,
                this,
                &GraphNodeWidget::on_node_reload_request);

  this->connect(this,
                &gngui::GraphViewer::node_right_clicked,
                this,
                &GraphNodeWidget::on_node_right_clicked);

  this->connect(this,
                &gngui::GraphViewer::nodes_copy_request,
                this,
                &GraphNodeWidget::on_nodes_copy_request);

  this->connect(this,
                &gngui::GraphViewer::nodes_duplicate_request,
                this,
                &GraphNodeWidget::on_nodes_duplicate_request);

  this->connect(this,
                &gngui::GraphViewer::nodes_paste_request,
                this,
                &GraphNodeWidget::on_nodes_paste_request);

  // Auto-wiring: node dropped on link
  this->connect(this,
                &gngui::GraphViewer::node_dropped_on_link_request,
                this,
                &GraphNodeWidget::on_node_dropped_on_link);

  // Undo / Redo
  this->connect(this,
                &gngui::GraphViewer::undo_request,
                this,
                &GraphNodeWidget::on_undo_request);

  this->connect(this,
                &gngui::GraphViewer::redo_request,
                this,
                &GraphNodeWidget::on_redo_request);

  // viewers
  this->connect(this,
                &gngui::GraphViewer::viewport_request,
                this,
                &GraphNodeWidget::on_viewport_request);

  // GraphNodeWidget -> QApplication
  this->connect(this,
                &GraphNodeWidget::update_started,
                this,
                []() { QApplication::setOverrideCursor(Qt::WaitCursor); });

  this->connect(this,
                &GraphNodeWidget::update_finished,
                this,
                []() { QApplication::restoreOverrideCursor(); });

  // GraphNodeWidget -> GFX node
  this->connect(this,
                &GraphNodeWidget::compute_finished,
                this,
                [this](const std::string &node_id)
                {
                  if (HSD_CTX.app_settings.interface.enable_node_settings_in_node_body)
                  {
                    // force update of the graphics node to update the node settings
                    // content
                    gngui::GraphicsNode *p_gfx_node = this->get_graphics_node_by_id(
                        node_id);
                    if (p_gfx_node)
                      p_gfx_node->update();
                  }
                });

  // GraphNode
  gno->update_started = [safe_this = QPointer(this)]()
  {
    if (safe_this)
      Q_EMIT safe_this->update_started();
  };

  gno->update_finished = [safe_this = QPointer(this)]()
  {
    if (safe_this)
      Q_EMIT safe_this->update_finished();
  };

  gno->compute_started = [safe_this = QPointer(this)](const std::string &node_id)
  {
    if (safe_this)
      Q_EMIT safe_this->compute_started(node_id);
  };

  gno->compute_finished = [safe_this = QPointer(this)](const std::string &node_id)
  {
    if (safe_this)
      Q_EMIT safe_this->compute_finished(node_id);
  };
}

} // namespace hesiod
