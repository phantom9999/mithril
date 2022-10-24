#pragma once

#include <cstdint>
#include <functional>

struct Entry {
  int32_t key{0};
  int32_t value{0};
  Entry* next{nullptr};
};

class HashMap {
 public:
  explicit HashMap(size_t size) {
    size_ = size;
    p_entry_ = new Entry*[size]{nullptr};
  }

  ~HashMap() {
    for (int i=0; i< size_; ++i) {
      auto entry = p_entry_[i];
      while (entry!= nullptr) {
        auto tmp = entry->next;
        delete entry;
        entry = tmp;
      }
    }
    delete[] p_entry_;
  }

  bool get(int32_t key, int32_t* value) {
    auto index = hash_(key) % size_;
    auto& entry = p_entry_[index];
    while (entry != nullptr) {
      if (entry->key == key) {
        *value = entry->value;
        return true;
      }
      entry = entry->next;
    }
    return false;
  }

  void set(int32_t key, int32_t value) {
    auto index = hash_(key) % size_;
    bool found = false;
    auto entry = p_entry_[index];
    while (entry != nullptr) {
      if (entry->key == key) {
        entry->value = value;
        found = true;
        break;
      }
      if (entry->next == nullptr && !found) {
        entry->next = new Entry;
        entry = entry->next;
        entry->key = key;
        entry->value = value;
        entry->next = nullptr;
        found = true;
        break;
      }
      entry = entry->next;
    }
    if (!found) {
      auto* tmp = new Entry;
      tmp->key = key;
      tmp->value = value;
      tmp->next = nullptr;
      p_entry_[index] = tmp;
    }
  }

 private:
  Entry** p_entry_{nullptr};
  size_t size_;
  std::hash<int32_t> hash_;
};
