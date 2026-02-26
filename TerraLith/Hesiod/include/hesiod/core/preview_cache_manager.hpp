/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace hesiod
{

// Smart preview cache for Hesiod 0.6.
// Stores computed node output data (as raw float buffers) in an LRU cache
// so that selecting a node shows its preview instantly (0-5 ms) instead
// of triggering a full recompute.
class PreviewCacheManager
{
public:
  static PreviewCacheManager &instance();

  // Store a preview for a node (copies data into cache)
  void store(const std::string &node_id, const std::vector<float> &data);

  // Retrieve cached preview (returns empty vector if not cached)
  std::vector<float> retrieve(const std::string &node_id) const;

  // Check if a node has a cached preview
  bool has_cache(const std::string &node_id) const;

  // Invalidate cache for a specific node and its downstream dependents
  void invalidate(const std::string &node_id);

  // Invalidate a node and all nodes in the given downstream list
  void invalidate_chain(const std::string              &node_id,
                        const std::vector<std::string> &downstream_ids);

  // Force refresh a single node's preview
  void force_refresh(const std::string &node_id);

  // Clear the entire cache
  void clear();

  // Get cache statistics
  float get_hit_rate() const;
  float get_memory_usage_mb() const;
  int   get_cache_percentage() const;

  // Set memory limit (MB)
  void set_memory_limit_mb(int limit_mb);

  // Get last preview retrieval time in ms
  float get_last_retrieval_time_ms() const;

private:
  PreviewCacheManager() = default;
  PreviewCacheManager(const PreviewCacheManager &) = delete;
  PreviewCacheManager &operator=(const PreviewCacheManager &) = delete;

  // LRU eviction support
  void evict_if_needed();
  void touch(const std::string &node_id) const;

  struct CacheEntry
  {
    std::vector<float> data;
    size_t             size_bytes = 0;
  };

  mutable std::unordered_map<std::string, CacheEntry> cache_;
  mutable std::list<std::string>                       lru_order_;
  mutable std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;
  mutable std::mutex                                   mutex_;

  size_t  memory_limit_bytes_ = 512 * 1024 * 1024; // 512 MB default
  size_t  current_memory_bytes_ = 0;
  mutable size_t  cache_hits_ = 0;
  mutable size_t  cache_misses_ = 0;
  mutable float   last_retrieval_ms_ = 0.0f;
};

} // namespace hesiod
