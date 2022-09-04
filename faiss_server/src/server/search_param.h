#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "index_constants.pb.h"

struct SearchParam {
  proto::Constants::ModelName model_name;
  proto::Constants::IndexType index_type;
  uint32_t query_size;
  uint32_t topk;
  uint32_t vec_size;
  const float* vec;
};
struct SearchResult {
  uint32_t batch_size;
  uint32_t size_per_batch;
  uint64_t version;
  std::vector<std::string> labels;
  std::vector<float> scores;
};

enum class SearchStatus {
  OK = 1,
  MODEL_NOT_FOUND,
  INDEX_NOT_FOUND,
  DIM_ERROR,
  FAISS_ERROR,
};

std::string ToString(SearchStatus status);
