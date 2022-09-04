#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include "server_config.pb.h"
#include "index_constants.pb.h"

#include "server/search_param.h"
#include "server/index_wrapper.h"

class ModelIndex : public boost::noncopyable {
 public:
  boost::atomic_shared_ptr<IndexWrapper> index_;
  std::atomic_uint64_t version_;
};

class IndexManager {
 public:

  IndexManager();

  bool Init(const proto::IndexConfig& index_config);

  bool Stop();

  SearchStatus Search(const SearchParam& param, SearchResult* result);

  bool GetStatus(proto::Constants::ModelName model_name, uint64_t* version, std::vector<std::tuple<proto::Constants::IndexType, uint64_t, uint64_t>>* status);

 private:
  void LoadModel();

 private:
  std::vector<ModelIndex> mulit_index_;
  std::vector<proto::ModelConfig> model_configs_;
  std::string index_path_;
  std::thread worker_;
  std::atomic_bool running_{true};
};

