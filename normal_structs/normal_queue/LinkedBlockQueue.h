#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

struct Node {
  int val;
  Node* next{nullptr};
  explicit Node(int value = 0): val(value) { }
};

class LinkedBlockQueue {
 public:
  LinkedBlockQueue(size_t size): capacity_(size) {
    head_ = new Node;
    tail_ = head_;
  }

  bool TryPush(int value) {
    if (count_.load() >= capacity_) {
      return false;
    }
    std::unique_lock lock(push_mutex_);
    Enqueue(value);
    auto current = count_.fetch_add(1);
    if (current + 1 < capacity_) {
      not_full_.notify_one();
    }
    if (current == 0) {
      NotifyNotEmpty();
    }

    return true;
  }

  bool TryPop(int* value) {
    if (value == nullptr) {
      return false;
    }
    if (count_.load() == 0) {
      return false;
    }
    std::unique_lock lock(pop_mutex_);
    Dequeue(value);
    auto current = count_.fetch_sub(1);
    if (current - 1 == 0) {
      not_empty_.notify_one();
    }
    if (current == capacity_) {
      NotifyNotFull();
    }

    return true;
  }

  void Push(int value) {
    size_t current = 0;
    {
      std::unique_lock lock(push_mutex_);
      not_full_.wait(lock, [&](){ return count_.load() < capacity_;});
      Enqueue(value);
      current = count_.fetch_add(1);
      if (current - 1 < capacity_) {
        not_full_.notify_one();
      }
    }
    if (current == 0) {
      NotifyNotEmpty();
    }
  }

  void Pop(int* value) {
    if (value == nullptr) {
      return;
    }
    size_t current = 0;
    {
      std::unique_lock lock(pop_mutex_);
      not_empty_.wait(lock, [&](){ return count_.load() > 0; });
      Dequeue(value);
      current = count_.fetch_sub(1);
      if (current > 1) {
        not_empty_.notify_one();
      }
    }
    if (current == capacity_) {
      NotifyNotFull();
    }
  }

 private:
  void Enqueue(int value) {
    tail_->next = new Node(value);
    tail_ = tail_->next;
  }

  void Dequeue(int* value) {
    auto* node = head_->next;
    *value = node->val;
    head_->next = node->next;
    delete node;
  }

  void NotifyNotFull() {
    std::unique_lock lock(push_mutex_);
    not_full_.notify_one();
  }

  void NotifyNotEmpty() {
    std::unique_lock lock(pop_mutex_);
    not_empty_.notify_one();
  }

 private:
  Node* head_;
  Node* tail_;
  std::mutex push_mutex_;
  std::mutex pop_mutex_;
  std::condition_variable not_full_;
  std::condition_variable not_empty_;
  const size_t capacity_;
  std::atomic_size_t count_{0};
};
