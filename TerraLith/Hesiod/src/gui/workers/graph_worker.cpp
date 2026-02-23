/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>

#include "hesiod/gui/workers/graph_worker.hpp"
#include "hesiod/logger.hpp"
#include "hesiod/model/graph/graph_node.hpp"
#include "hesiod/model/nodes/base_node.hpp"

namespace hesiod
{

GraphWorker::GraphWorker(QObject *parent) : QObject(parent) {}

void GraphWorker::configure(GraphNode                      *p_graph,
                            const std::vector<std::string> &sorted_node_ids)
{
  this->p_graph_ = p_graph;
  this->sorted_ids_ = sorted_node_ids;
  this->cancel_requested_.store(false);
}

void GraphWorker::request_cancel() { this->cancel_requested_.store(true); }

bool GraphWorker::is_cancel_requested() const
{
  return this->cancel_requested_.load();
}

void GraphWorker::do_compute()
{
  Logger::log()->trace("GraphWorker::do_compute: starting {} nodes",
                       this->sorted_ids_.size());

  bool cancelled = false;
  int  total = static_cast<int>(this->sorted_ids_.size());

  for (int i = 0; i < total; ++i)
  {
    if (this->cancel_requested_.load())
    {
      Logger::log()->info("GraphWorker::do_compute: cancelled at node {}/{}", i, total);
      cancelled = true;
      break;
    }

    const std::string &nid = this->sorted_ids_[i];

    // Progress before compute
    float progress = (total > 1)
                         ? 100.f * static_cast<float>(i) / static_cast<float>(total)
                         : 0.f;
    Q_EMIT this->progress_updated(nid, progress);

    // Signal: compute started
    Q_EMIT this->node_compute_started(nid);

    // Execute the node
    auto t0 = std::chrono::steady_clock::now();

    gnode::Node *p_node = this->p_graph_->get_node_ref_by_id(nid);
    if (p_node)
    {
      p_node->is_dirty = true;
      p_node->update();
    }

    auto  t1 = std::chrono::steady_clock::now();
    float elapsed_ms =
        std::chrono::duration<float, std::milli>(t1 - t0).count();

    // Signal: compute finished + execution time
    Q_EMIT this->node_compute_finished(nid);
    Q_EMIT this->node_execution_time(nid, elapsed_ms);

    // Progress after compute
    float progress_after =
        (total > 1)
            ? 100.f * static_cast<float>(i + 1) / static_cast<float>(total)
            : 100.f;
    Q_EMIT this->progress_updated(nid, progress_after);
  }

  Q_EMIT this->compute_all_finished(cancelled);
}

} // namespace hesiod
