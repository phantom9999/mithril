#pragma once

#include "builder_config.pb.h"
#include "index_constants.pb.h"

class BuildTask {
 public:
  BuildTask(const proto::TaskConfig& task_config, const std::string& output_path);

  bool BuildIndex();

 private:
  // bool ReadBinaryStream(const std::string& path);
  bool ReadBinary(const std::string& path);

  bool WriteIds(const std::string& path);
  bool WriteMultiIndex(const std::string& model_dir, const std::vector<int>& index_types);

 private:
  proto::TaskConfig task_config_;
  std::string output_path_;
  uint32_t dim_{0};
  uint32_t length_{0};
  std::vector<std::string> labels_;
  std::vector<float> matrix_;
  std::unordered_map<proto::Constants::IndexType, std::string> type_to_keys_;
};


