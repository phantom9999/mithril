#define BOOST_TEST_MODULE torch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <queue>
#include <vector>
#include <algorithm>
#include <iostream>
#include <set>

BOOST_AUTO_TEST_CASE(version) {
  std::vector<int64_t> data{5,2,7,9,1,4,2};
  std::priority_queue<int64_t, std::vector<int64_t>, std::greater<>> que(data.begin(), data.end());
  while (!que.empty()) {
    std::cout << que.top() << ",";
    que.pop();
  }
  std::cout << std::endl;

  std::set<int64_t, std::greater<>> data_set(data.begin(), data.end());
  std::cout << data_set.begin().operator*() << std::endl;

  std::sort(data.begin(), data.end(), std::greater<>());
  for (const auto& item : data) {
    std::cout << item << ",";
  }
  std::cout << std::endl;
}

