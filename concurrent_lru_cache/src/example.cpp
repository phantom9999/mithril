#include <iostream>
#include <string>
#include "concurrent-scalable-cache.h"

int main() {
  HPHP::ConcurrentScalableCache<std::string, std::string> lru_cache{1024, 8};
  for (int i=0;i<10;++i) {
    lru_cache.insert(std::to_string(i), std::to_string(i));
  }

}
