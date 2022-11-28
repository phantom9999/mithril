#pragma once

void largeSort(std::vector<int>& data, std::vector<int>& res) {
  std::bitset<100> res_bit;
  for (const auto& item : data) {
    res_bit.set(item);
  }
  res.clear();
  for (int i=0; i<res_bit.size(); ++i) {
    if (res_bit.test(i)) {
      res.push_back(i);
    }
  }
}

