#include <iostream>
#include <vector>
#include "quick_sort.h"
#include <algorithm>

int main() {
  std::vector<int32_t> data{83,86,77,15,93,35,86,92,49,21};
  /*for (int i=0; i< 10; ++i) {
    data.push_back(rand()%100);
  }*/
  for (const auto& item : data) {
    std:: cout << item << ",";
  }
  std::cout << std::endl;
  // quickSort(data, 0, data.size()-1);
  std::sort(data.begin(), data.end(), [](const int& left, const int& right){
    return left < right;
  });

  for (const auto& item : data) {
    std:: cout << item << ",";
  }
  std::cout << std::endl;
}
