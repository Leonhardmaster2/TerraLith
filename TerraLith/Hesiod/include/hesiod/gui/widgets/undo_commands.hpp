/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <map>
#include <string>
#include <vector>

#include <QPointF>
#include <QPointer>
#include <QUndoCommand>

#include "nlohmann/json.hpp"

namespace hesiod
{

class GraphNodeWidget; // forward

// Command ID enum for mergeWith() support
enum UndoCommandId
{
  CMD_DELETE_NODES = 1000,
  CMD_ADD_NODE,
  CMD_ADD_LINK,
  CMD_REMOVE_LINK,
  CMD_MOVE_NODES,
  CMD_CHANGE_PROPERTY,    // property/attribute value change on a node
  CMD_PASTE_NODES,        // paste / duplicate / import nodes
  CMD_DROP_NODE_ON_LINK,  // insert node into existing link
};

// =====================================
// DeleteNodesCommand
// =====================================
// Captures selected nodes + ALL connected links (internal AND external) as JSON.
// On redo: deletes the nodes.
// On undo: recreates nodes with original IDs, restores attributes and all links
//          (verifying that external link endpoints still exist before reconnecting).
class DeleteNodesCommand : public QUndoCommand
{
public:
  DeleteNodesCommand(GraphNodeWidget         *widget,
                     nlohmann::json           snapshot,
                     std::vector<std::string> node_ids,
                     QUndoCommand            *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_DELETE_NODES; }

private:
  QPointer<GraphNodeWidget> widget;
  nlohmann::json            snapshot;   // nodes + internal_links + external_links
  std::vector<std::string>  node_ids;
  bool                      first_redo = true;
};

// =====================================
// AddNodeCommand
// =====================================
// Captures a newly created node. On undo: deletes it. On redo: recreates it
// with the same ID.
class AddNodeCommand : public QUndoCommand
{
public:
  AddNodeCommand(GraphNodeWidget *widget,
                 const std::string &node_id,
                 const std::string &node_type,
                 QPointF            scene_pos,
                 QUndoCommand      *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_ADD_NODE; }

private:
  QPointer<GraphNodeWidget> widget;
  std::string               node_id;
  std::string               node_type;
  QPointF                   scene_pos;
  nlohmann::json            settings_snapshot; // captured on first undo
  bool                      first_redo = true;
};

// =====================================
// AddLinkCommand
// =====================================
// Captures a newly created link. On undo: removes it. On redo: recreates it.
class AddLinkCommand : public QUndoCommand
{
public:
  AddLinkCommand(GraphNodeWidget   *widget,
                 const std::string &id_out,
                 const std::string &port_id_out,
                 const std::string &id_in,
                 const std::string &port_id_in,
                 QUndoCommand      *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_ADD_LINK; }

private:
  QPointer<GraphNodeWidget> widget;
  std::string               id_out;
  std::string               port_id_out;
  std::string               id_in;
  std::string               port_id_in;
  bool                      first_redo = true;
};

// =====================================
// RemoveLinkCommand
// =====================================
// Captures a removed link. On undo: recreates it. On redo: removes it.
class RemoveLinkCommand : public QUndoCommand
{
public:
  RemoveLinkCommand(GraphNodeWidget   *widget,
                    const std::string &id_out,
                    const std::string &port_id_out,
                    const std::string &id_in,
                    const std::string &port_id_in,
                    QUndoCommand      *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_REMOVE_LINK; }

private:
  QPointer<GraphNodeWidget> widget;
  std::string               id_out;
  std::string               port_id_out;
  std::string               id_in;
  std::string               port_id_in;
  bool                      first_redo = true;
};

// =====================================
// MoveNodesCommand
// =====================================
// Captures node position changes. Uses mergeWith() to coalesce consecutive
// moves of the same set of nodes into a single undo step.
class MoveNodesCommand : public QUndoCommand
{
public:
  MoveNodesCommand(GraphNodeWidget                       *widget,
                   std::map<std::string, QPointF>         old_positions,
                   std::map<std::string, QPointF>         new_positions,
                   QUndoCommand                          *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_MOVE_NODES; }
  bool mergeWith(const QUndoCommand *other) override;

private:
  QPointer<GraphNodeWidget>      widget;
  std::map<std::string, QPointF> old_positions;
  std::map<std::string, QPointF> new_positions;
  bool                           first_redo = true;
};

// =====================================
// PropertyChangeCommand
// =====================================
// Captures attribute changes on a single node. Stores before/after attribute
// JSON snapshots (attribute keys only, no node metadata).
// On undo: restores old attributes and triggers recompute.
// On redo: restores new attributes and triggers recompute.
class PropertyChangeCommand : public QUndoCommand
{
public:
  PropertyChangeCommand(GraphNodeWidget   *widget,
                        const std::string &node_id,
                        nlohmann::json     old_attrs,
                        nlohmann::json     new_attrs,
                        QUndoCommand      *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_CHANGE_PROPERTY; }

private:
  QPointer<GraphNodeWidget> widget;
  std::string               node_id;
  nlohmann::json            old_attrs;
  nlohmann::json            new_attrs;
  bool                      first_redo = true;
};

// =====================================
// PasteNodesCommand
// =====================================
// Captures a paste/duplicate/import operation. Stores the list of created node
// IDs. On first undo a full snapshot (nodes + links) is captured so that
// subsequent redo can restore with the SAME IDs (json_import would generate new
// IDs, breaking any later commands that reference the originals).
// On undo: captures snapshot, then deletes all pasted nodes.
// On redo: restores from snapshot (preserving original IDs).
class PasteNodesCommand : public QUndoCommand
{
public:
  PasteNodesCommand(GraphNodeWidget         *widget,
                    std::vector<std::string> created_node_ids,
                    QUndoCommand            *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_PASTE_NODES; }

private:
  QPointer<GraphNodeWidget> widget;
  std::vector<std::string>  created_node_ids;
  nlohmann::json            snapshot;  // captured on first undo for ID-preserving redo
  bool                      first_redo = true;
};

// =====================================
// DropNodeOnLinkCommand
// =====================================
// Captures a node-dropped-on-link operation (remove 1 link, create 2 links).
// On undo: removes the 2 new links and recreates the original link.
// On redo: removes the original link and creates the 2 new links.
class DropNodeOnLinkCommand : public QUndoCommand
{
public:
  DropNodeOnLinkCommand(GraphNodeWidget   *widget,
                        const std::string &original_out_id,
                        const std::string &original_out_port,
                        const std::string &original_in_id,
                        const std::string &original_in_port,
                        const std::string &dropped_node_id,
                        const std::string &dropped_in_port,
                        const std::string &dropped_out_port,
                        QUndoCommand      *parent = nullptr);

  void undo() override;
  void redo() override;
  int  id() const override { return CMD_DROP_NODE_ON_LINK; }

private:
  QPointer<GraphNodeWidget> widget;
  // Original link that was broken
  std::string original_out_id;
  std::string original_out_port;
  std::string original_in_id;
  std::string original_in_port;
  // Dropped node and its ports
  std::string dropped_node_id;
  std::string dropped_in_port;
  std::string dropped_out_port;
  bool        first_redo = true;
};

} // namespace hesiod
