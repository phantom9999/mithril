#pragma once

#include <vector>
#include <cstdint>

static void quickSort(std::vector<int32_t>& data, int begin, int end) {
  if (begin >= end) {
    return;
  }
  int current = data[begin];
  int left = begin;
  int right = end;
  while (left < right) {
    while (data[right] >= current && left < right) {
      --right;
    }
    while (data[left] <= current && left < right) {
      ++left;
    }
    if (left < right) {
      int tmp = data[left];
      data[left] = data[right];
      data[right] = tmp;
    }
  }
  data[begin] = data[left];
  data[left] = current;
  quickSort(data, begin, left-1);
  quickSort(data, left+1, end);
}
