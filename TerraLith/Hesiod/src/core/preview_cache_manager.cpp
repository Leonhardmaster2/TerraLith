/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <chrono>

#include "hesiod/core/preview_cache_manager.hpp"
#include "hesiod/logger.hpp"

namespace hesiod
{

PreviewCacheManager &PreviewCacheManager::instance()
{
  static PreviewCacheManager inst;
  return inst;
}

void PreviewCacheManager::store(const std::string        &node_id,
                                const std::vector<float> &data)
{
  std::lock_guard<std::mutex> lock(mutex_);

  size_t new_size = data.size() * sizeof(float);

  // Remove old entry if exists
  if (cache_.count(node_id))
  {
    current_memory_bytes_ -= cache_[node_id].size_bytes;

    // Remove from LRU
    if (lru_map_.count(node_id))
    {
      lru_order_.erase(lru_map_[node_id]);
      lru_map_.erase(node_id);
    }
  }

  // Store new entry
  cache_[node_id] = CacheEntry{data, new_size};
  current_memory_bytes_ += new_size;

  // Update LRU
  lru_order_.push_front(node_id);
  lru_map_[node_id] = lru_order_.begin();

  // Evict if over limit
  evict_if_needed();
}

std::vector<float> PreviewCacheManager::retrieve(const std::string &node_id) const
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto start = std::chrono::high_resolution_clock::now();

  auto it = cache_.find(node_id);
  if (it == cache_.end())
  {
    cache_misses_++;
    last_retrieval_ms_ = 0.0f;
    return {};
  }

  cache_hits_++;
  touch(node_id);

  auto end = std::chrono::high_resolution_clock::now();
  last_retrieval_ms_ =
      std::chrono::duration<float, std::milli>(end - start).count();

  return it->second.data;
}

bool PreviewCacheManager::has_cache(const std::string &node_id) const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return cache_.count(node_id) > 0;
}

void PreviewCacheManager::invalidate(const std::string &node_id)
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = cache_.find(node_id);
  if (it != cache_.end())
  {
    current_memory_bytes_ -= it->second.size_bytes;

    if (lru_map_.count(node_id))
    {
      lru_order_.erase(lru_map_[node_id]);
      lru_map_.erase(node_id);
    }

    cache_.erase(it);
  }
}

void PreviewCacheManager::invalidate_chain(
    const std::string              &node_id,
    const std::vector<std::string> &downstream_ids)
{
  invalidate(node_id);
  for (const auto &id : downstream_ids)
    invalidate(id);
}

void PreviewCacheManager::force_refresh(const std::string &node_id)
{
  invalidate(node_id);
}

void PreviewCacheManager::clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  cache_.clear();
  lru_order_.clear();
  lru_map_.clear();
  current_memory_bytes_ = 0;
  cache_hits_ = 0;
  cache_misses_ = 0;
}

float PreviewCacheManager::get_hit_rate() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  size_t total = cache_hits_ + cache_misses_;
  if (total == 0)
    return 0.0f;
  return static_cast<float>(cache_hits_) / static_cast<float>(total) * 100.0f;
}

float PreviewCacheManager::get_memory_usage_mb() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<float>(current_memory_bytes_) / (1024.0f * 1024.0f);
}

int PreviewCacheManager::get_cache_percentage() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (memory_limit_bytes_ == 0)
    return 0;
  return static_cast<int>(100.0 * static_cast<double>(current_memory_bytes_) /
                          static_cast<double>(memory_limit_bytes_));
}

void PreviewCacheManager::set_memory_limit_mb(int limit_mb)
{
  std::lock_guard<std::mutex> lock(mutex_);
  memory_limit_bytes_ = static_cast<size_t>(limit_mb) * 1024 * 1024;
  evict_if_needed();
}

float PreviewCacheManager::get_last_retrieval_time_ms() const
{
  return last_retrieval_ms_;
}

void PreviewCacheManager::evict_if_needed()
{
  while (current_memory_bytes_ > memory_limit_bytes_ && !lru_order_.empty())
  {
    const std::string &victim = lru_order_.back();

    auto it = cache_.find(victim);
    if (it != cache_.end())
    {
      current_memory_bytes_ -= it->second.size_bytes;
      cache_.erase(it);
    }

    lru_map_.erase(victim);
    lru_order_.pop_back();
  }
}

void PreviewCacheManager::touch(const std::string &node_id) const
{
  if (lru_map_.count(node_id))
  {
    lru_order_.erase(lru_map_[node_id]);
    lru_order_.push_front(node_id);
    lru_map_[node_id] = lru_order_.begin();
  }
}

} // namespace hesiod
