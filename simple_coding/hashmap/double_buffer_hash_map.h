#pragma once

#include <atomic>
#include <mutex>
#include <memory>
#include <absl/container/flat_hash_map.h>

using HashMap = absl::flat_hash_map<int32_t, int32_t>;
using HashMapPtr = std::shared_ptr<HashMap>;

/**
 * 使用双buffer实现的支持并发读写的哈希表
 */
class DoubleBufferHashMap {
 public:
  DoubleBufferHashMap() {
    readbuffer_[0] = std::make_shared<HashMap>();
    readbuffer_[1] = nullptr;
  }

  bool read(int32_t key, int32_t* value) {
    if (value == nullptr) {
      return false;
    }
    auto buffer = readbuffer_[index_.load()];
    auto it = buffer->find(key);
    if (it == buffer->end()) {
      return false;
    }
    *value = it->second;
    return true;
  }

  void set(int32_t key, int32_t value) {
    std::lock_guard guard{mutex_};
    writebuffer_.emplace(key, value);
    if (!needChange()) {
      return;
    }
    int32_t index = !index_.load();
    readbuffer_[index] = std::make_shared<HashMap>(writebuffer_.begin(), writebuffer_.end());
    index_.store(index);
    readbuffer_[!index] = nullptr;
  }

  /**
   * 自定义更新策略
   * @return
   */
  bool needChange() {
    return false;
  }

 private:
  std::atomic_int index_{0};
  HashMapPtr readbuffer_[2];
  HashMap writebuffer_;
  std::mutex mutex_;
};
