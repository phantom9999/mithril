#include <unordered_map>

int main() {
  std::unordered_map<int32_t, int32_t> data1;
  data1.insert(std::make_pair(2, 3));
  data1.insert(std::pair(2, 4));
}

