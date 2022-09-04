#pragma once

#include <string>
#include <vector>
#include <faiss/Index.h>

#include "server/search_param.h"
#include "index_constants.pb.h"

class IndexWrapper {
 public:
  IndexWrapper();

  bool Init(const std::string& path);

  SearchStatus Search(const SearchParam& param, SearchResult* result);

  bool Status(std::vector<std::tuple<proto::Constants::IndexType, uint64_t, uint64_t>>* status);

 private:
  bool LoadIds(const std::string& path);

  bool LoadFaiss(const std::string& path);

 private:
  std::vector<std::string> labels_;
  std::vector<std::unique_ptr<faiss::Index>> indexes_;
};
