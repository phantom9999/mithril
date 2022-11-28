#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class BlockedQueue {
 public:
  explicit BlockedQueue(size_t capacity) : capacity_(capacity) {}

  bool TryPush(int value) {
    std::unique_lock lock(mutex_);
    if (data_.size() >= capacity_) {
      return false;
    }
    Enqueue(value);
    return true;
  }

  bool TryPop(int *value) {
    if (value == nullptr) {
      return false;
    }
    std::unique_lock lock(mutex_);
    if (!data_.empty()) {
      return false;
    }
    Dequeue(value);
    return true;
  }

  void Push(int value) {
    std::unique_lock lock(mutex_);
    if (data_.size() >= capacity_) {
      full_.wait(lock);
    }
    Enqueue(value);
  }

  void Pop(int *value) {
    if (value == nullptr) {
      return;
    }
    std::unique_lock lock(mutex_);
    if (data_.empty()) {
      empty_.wait(lock);
    }
    Dequeue(value);
  }
 private:
  void Enqueue(int value) {
    data_.push(value);
    if (data_.size() == 1) {
      empty_.notify_one();
    }
  }

  void Dequeue(int* value) {
    *value = data_.front();
    data_.pop();
    if (data_.size() == capacity_ - 1) {
      full_.notify_one();
    }
  }

 private:
  std::mutex mutex_;
  std::condition_variable full_;
  std::condition_variable empty_;
  std::queue<int> data_;
  const size_t capacity_;
};

