#include <queue>
#include <iostream>
#include <vector>


int main() {
  std::priority_queue<int32_t, std::vector<int32_t>, std::less<>> pq;
  pq.push(12);
  pq.push(1);
  pq.push(120);
  while (!pq.empty()) {
    std::cout << pq.top() << " ";
    pq.pop();
  }
  std::cout << std::endl;
}
