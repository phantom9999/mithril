#include <iostream>
#include "custom_hash_map.h"

int main() {
  HashMap* pp{nullptr};
  {
    HashMap hash_map{100};
    pp = &hash_map;
    hash_map.set(10, 100);
    hash_map.set(1, 50);
    {
      int tmp{0};
      hash_map.get(10, &tmp);
      std::cout << tmp << std::endl;
    }
    {
      int tmp{0};
      hash_map.get(1, &tmp);
      std::cout << tmp << std::endl;
    }
  }
  int a = 0;
  a += 1;
}
