/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#pragma once
#include <atomic>
#include <string>
#include <vector>

#include <QObject>

namespace hesiod
{

class GraphNode;

class GraphWorker : public QObject
{
  Q_OBJECT

public:
  explicit GraphWorker(QObject *parent = nullptr);

  void configure(GraphNode                      *p_graph,
                 const std::vector<std::string> &sorted_node_ids);

  void request_cancel();
  bool is_cancel_requested() const;

public slots:
  void do_compute();

signals:
  void node_compute_started(const std::string &node_id);
  void node_compute_finished(const std::string &node_id);
  void progress_updated(const std::string &node_id, float progress_percent);
  void node_execution_time(const std::string &node_id, float time_ms, int backend_type);
  void compute_all_finished(bool was_cancelled);

private:
  GraphNode                *p_graph_ = nullptr;
  std::vector<std::string>  sorted_ids_;
  std::atomic<bool>         cancel_requested_{false};
};

} // namespace hesiod
