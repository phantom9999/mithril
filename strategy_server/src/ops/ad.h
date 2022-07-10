#pragma once

#include <cstdint>
#include <vector>

struct Ad {
  uint64_t id;
  float score;
};

class AdList : public std::vector<Ad> {

};
