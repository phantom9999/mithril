#include "server/index_wrapper.h"

#include <fstream>

#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <faiss/Index.h>
#include <faiss/index_io.h>
#include <absl/strings/str_cat.h>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>

#include "common/path.h"
#include "common/constants.h"

bool IndexWrapper::Init(const std::string &path) {
  std::string ids_file = absl::StrCat(path, "/", kFaissIdsName);

  if (!LoadIds(ids_file)) {
    return false;
  }

  if (!LoadFaiss(path)) {
    return false;
  }

  std::vector<std::string> debugs;
  for (int i=0;i<this->indexes_.size();++i) {
    if (indexes_[i] == nullptr) {
      continue;
    }
    auto type = static_cast<proto::Constants::IndexType>(i);
    debugs.push_back(absl::StrCat(proto::Constants::IndexType_Name(type), "<", indexes_[i]->ntotal, ",", indexes_[i]->d, ">"));
  }
  LOG(INFO) << "load " << path << " success, " << debugs;
  return true;
}

SearchStatus IndexWrapper::Search(const SearchParam &param, SearchResult *result) {
  if (indexes_[param.index_type] == nullptr) {
    return SearchStatus::INDEX_NOT_FOUND;
  }
  auto& index = indexes_[param.index_type];

  if (index->d * param.query_size != param.vec_size) {
    return SearchStatus::DIM_ERROR;
  }

  uint32_t result_size = param.query_size * param.topk;
  std::vector<faiss::idx_t> ids(result_size);
  std::vector<float> scores(result_size);
  try {
    index->search(param.query_size, param.vec, param.topk, scores.data(), ids.data());
  } catch (std::exception& e) {
    LOG(WARNING) << "search error: " << e.what();
    return SearchStatus::FAISS_ERROR;
  }

  result->scores = std::move(scores);
  result->labels.reserve(result_size);
  for (const auto& id : ids) {
    result->labels.push_back(this->labels_[id]);
  }

  result->batch_size = param.query_size;
  result->size_per_batch = param.topk;
  return SearchStatus::OK;
}
bool IndexWrapper::LoadIds(const std::string &path) {
  std::ifstream reader{path};
  if (!reader.is_open()) {
    LOG(WARNING) << path << " not found";
    return false;
  }
  std::string line;
  while (std::getline(reader, line)) {
    labels_.push_back(std::move(line));
  }
  return true;
}
bool IndexWrapper::LoadFaiss(const std::string &path) {
  using proto::Constants;
  std::unordered_map<std::string, Constants::IndexType> file_to_types;
  for (int i=Constants::IndexType_MIN; i< Constants::IndexType_ARRAYSIZE; ++i) {
    if (!Constants::IndexType_IsValid(i)) {
      continue;
    }
    auto index_type = static_cast<Constants::IndexType>(i);
    file_to_types[GetIndexFileName(index_type)] = index_type;
  }
  boost::system::error_code ec;
  for (const auto& file : boost::filesystem::directory_iterator(path, ec)) {
    if (!is_regular_file(file, ec)) {
      continue;
    }
    auto filename = file.path().filename().string();
    auto it = file_to_types.find(filename);
    if (it == file_to_types.end()) {
      continue;
    }
    faiss::Index* index = nullptr;
    try {
      index = faiss::read_index(file.path().c_str());
    } catch (std::exception& e) {
      LOG(WARNING) << "load faiss index error: " << e.what();
      return false;
    }
    if (index == nullptr || index->ntotal != this->labels_.size()) {
      continue;
    }
    this->indexes_[it->second].reset(index);
  }
  return true;
}
IndexWrapper::IndexWrapper(): indexes_(proto::Constants::IndexType_ARRAYSIZE) {

}
bool IndexWrapper::Status(std::vector<std::tuple<proto::Constants::IndexType, uint64_t, uint64_t>> *status) {
  if (status == nullptr) {
    return false;
  }
  using proto::Constants;
  for (int i=Constants::IndexType_MIN;i<Constants::IndexType_ARRAYSIZE; ++i) {
    if (!Constants::IndexType_IsValid(i)) {
      continue;
    }
    auto index_type = static_cast<Constants::IndexType>(i);
    auto& index = indexes_[index_type];
    if (index == nullptr) {
      continue;
    }
    status->emplace_back(index_type, index->ntotal, index->d);
  }
  return true;
}
