#pragma once

#include <mutex>
#include <absl/container/flat_hash_map.h>

template<typename KeyType, typename ValueType>
struct Segment {
  absl::flat_hash_map<KeyType, ValueType> data;
  std::mutex mutex;
};

/**
 * 分段加锁的哈希表
 */
template<typename KeyType, typename ValueType, std::size_t SIZE = 10>
class SegmentHashMap {
 public:
  SegmentHashMap() = default;

  bool get(KeyType key, ValueType* value) {
    size_t index = hash_func_(key) % SIZE;
    auto& buffer = segment_[index];
    std::lock_guard guard{buffer.mutex};
    auto it = buffer.data.find(key);
    if (it == buffer.data.end()) {
      return false;
    }
    *value = it->second;
    return true;
  }

  void set(KeyType key, KeyType value) {
    size_t index = hash_func_(key) % SIZE;
    auto& buffer = segment_[index];
    std::lock_guard guard{buffer.mutex};
    buffer.data.emplace(key, value);
  }

 private:
  Segment<KeyType, ValueType> segment_[SIZE];
  std::hash<KeyType> hash_func_;
};
