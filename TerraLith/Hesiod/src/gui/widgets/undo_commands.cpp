/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "hesiod/gui/widgets/undo_commands.hpp"
#include "hesiod/gui/widgets/graph_node_widget.hpp"
#include "hesiod/model/graph/graph_node.hpp"
#include "hesiod/model/nodes/base_node.hpp"

#include "gnodegui/graphics_node.hpp"

namespace hesiod
{

// =====================================
// DeleteNodesCommand
// =====================================

DeleteNodesCommand::DeleteNodesCommand(GraphNodeWidget         *widget,
                                       nlohmann::json           snapshot,
                                       std::vector<std::string> node_ids,
                                       QUndoCommand            *parent)
    : QUndoCommand(parent), widget(widget), snapshot(std::move(snapshot)),
      node_ids(std::move(node_ids))
{
  setText("Delete Nodes");
}

void DeleteNodesCommand::undo()
{
  if (!widget)
    return;

  widget->restore_nodes_from_snapshot(snapshot);
}

void DeleteNodesCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  widget->delete_nodes_by_ids(node_ids);
}

// =====================================
// AddNodeCommand
// =====================================

AddNodeCommand::AddNodeCommand(GraphNodeWidget   *widget,
                               const std::string &node_id,
                               const std::string &node_type,
                               QPointF            scene_pos,
                               QUndoCommand      *parent)
    : QUndoCommand(parent), widget(widget), node_id(node_id), node_type(node_type),
      scene_pos(scene_pos)
{
  setText(QString("Add %1").arg(QString::fromStdString(node_type)));
}

void AddNodeCommand::undo()
{
  if (!widget)
    return;

  // Capture current settings before deleting (so redo can restore them)
  try
  {
    auto *gno = widget->get_p_graph_node();
    BaseNode *p_node = gno->get_node_ref_by_id<BaseNode>(node_id);
    if (p_node)
      settings_snapshot = p_node->json_to();
  }
  catch (...)
  {
    return;
  }

  widget->delete_nodes_by_ids({node_id});
}

void AddNodeCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  // Recreate the node with the original ID
  widget->create_node_with_id(node_type, node_id, scene_pos, settings_snapshot);
}

// =====================================
// AddLinkCommand
// =====================================

AddLinkCommand::AddLinkCommand(GraphNodeWidget   *widget,
                               const std::string &id_out,
                               const std::string &port_id_out,
                               const std::string &id_in,
                               const std::string &port_id_in,
                               QUndoCommand      *parent)
    : QUndoCommand(parent), widget(widget), id_out(id_out), port_id_out(port_id_out),
      id_in(id_in), port_id_in(port_id_in)
{
  setText("Add Link");
}

void AddLinkCommand::undo()
{
  if (!widget)
    return;

  widget->remove_link_internal(id_out, port_id_out, id_in, port_id_in);
}

void AddLinkCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  widget->create_link_internal(id_out, port_id_out, id_in, port_id_in);
}

// =====================================
// RemoveLinkCommand
// =====================================

RemoveLinkCommand::RemoveLinkCommand(GraphNodeWidget   *widget,
                                     const std::string &id_out,
                                     const std::string &port_id_out,
                                     const std::string &id_in,
                                     const std::string &port_id_in,
                                     QUndoCommand      *parent)
    : QUndoCommand(parent), widget(widget), id_out(id_out), port_id_out(port_id_out),
      id_in(id_in), port_id_in(port_id_in)
{
  setText("Remove Link");
}

void RemoveLinkCommand::undo()
{
  if (!widget)
    return;

  widget->create_link_internal(id_out, port_id_out, id_in, port_id_in);
}

void RemoveLinkCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  widget->remove_link_internal(id_out, port_id_out, id_in, port_id_in);
}

// =====================================
// MoveNodesCommand
// =====================================

MoveNodesCommand::MoveNodesCommand(GraphNodeWidget                *widget,
                                   std::map<std::string, QPointF>  old_positions,
                                   std::map<std::string, QPointF>  new_positions,
                                   QUndoCommand                   *parent)
    : QUndoCommand(parent), widget(widget), old_positions(std::move(old_positions)),
      new_positions(std::move(new_positions))
{
  setText("Move Nodes");
}

void MoveNodesCommand::undo()
{
  if (!widget)
    return;

  for (auto &[node_id, pos] : old_positions)
  {
    gngui::GraphicsNode *p_gfx = widget->get_graphics_node_by_id(node_id);
    if (p_gfx)
      p_gfx->setPos(pos);
  }
}

void MoveNodesCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  for (auto &[node_id, pos] : new_positions)
  {
    gngui::GraphicsNode *p_gfx = widget->get_graphics_node_by_id(node_id);
    if (p_gfx)
      p_gfx->setPos(pos);
  }
}

bool MoveNodesCommand::mergeWith(const QUndoCommand *other)
{
  if (other->id() != id())
    return false;

  const MoveNodesCommand *move_cmd = static_cast<const MoveNodesCommand *>(other);

  // Only merge if the same set of nodes is being moved
  if (old_positions.size() != move_cmd->old_positions.size())
    return false;

  for (auto &[node_id, _] : old_positions)
  {
    if (move_cmd->old_positions.find(node_id) == move_cmd->old_positions.end())
      return false;
  }

  // Merge: keep our old_positions (the original), take the other's new_positions
  new_positions = move_cmd->new_positions;
  return true;
}

// =====================================
// PropertyChangeCommand
// =====================================

PropertyChangeCommand::PropertyChangeCommand(GraphNodeWidget   *widget,
                                             const std::string &node_id,
                                             nlohmann::json     old_attrs,
                                             nlohmann::json     new_attrs,
                                             QUndoCommand      *parent)
    : QUndoCommand(parent), widget(widget), node_id(node_id),
      old_attrs(std::move(old_attrs)), new_attrs(std::move(new_attrs))
{
  setText("Change Property");
}

void PropertyChangeCommand::undo()
{
  if (!widget)
    return;

  widget->restore_node_attributes(node_id, old_attrs);
}

void PropertyChangeCommand::redo()
{
  if (!widget)
    return;

  if (first_redo)
  {
    first_redo = false;
    return;
  }

  widget->restore_node_attributes(node_id, new_attrs);
}

} // namespace hesiod
