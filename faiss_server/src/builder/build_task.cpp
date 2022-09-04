#include "build_task.h"
#include <fstream>

#include <absl/strings/str_cat.h>
#include <faiss/Index.h>
#include <faiss/index_io.h>
#include <faiss/index_factory.h>
#include <glog/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "source.pb.h"

#include "common/path.h"
#include "common/constants.h"
#include "common/timer.h"


BuildTask::BuildTask(const proto::TaskConfig &task_config, const std::string& output_path): task_config_(task_config), output_path_(output_path) {
  for (const auto& entry : index_type_names) {
    this->type_to_keys_[entry.first] = entry.second;
  }
}

bool BuildTask::BuildIndex() {
  std::string model_name = proto::Constants::ModelName_Name(task_config_.model_name());
  std::string model_dir = GenTimePath(output_path_, model_name);
  if (!MkdirIfNotExist(model_dir)) {
    LOG(WARNING) << "mkdir error " << model_dir;
    return false;
  }

  std::string source_file = absl::StrCat(task_config_.input_path(), "/", kSourceFileName, kSourceBinarySuffix);
  std::string ids_file = absl::StrCat(model_dir, "/", kFaissIdsName);

  Timer timer;
  if (!ReadBinary(source_file)) {
    LOG(WARNING) << "read source error";
    return false;
  }

  if (!WriteIds(ids_file)) {
    LOG(WARNING) << "write idsfile error";
    return false;
  }

  if (!WriteMultiIndex(model_dir, {task_config_.index_types().begin(), task_config_.index_types().end()})) {
    LOG(WARNING) << "write index error";
    return false;
  }
  LOG(INFO) << "build " << model_name << " success with <" << this->length_ << "," << this->dim_
    << ">, cost: " << timer.MsCost() << "ms";
  return true;
}

/*bool BuildTask::ReadBinaryStream(const std::string &path) {
  // 似乎有bug
  std::ifstream reader(path, std::ios::binary);
  if (!reader.is_open()) {
    LOG(WARNING) << "open " << path << " error";
    return false;
  }
  // 读取meta
  google::protobuf::io::IstreamInputStream istream_input_stream(&reader);
  google::protobuf::io::CodedInputStream coded_input_stream(&istream_input_stream);

  {
    uint64_t meta_len{0};
    if (!coded_input_stream.ReadVarint64(&meta_len) || meta_len == 0) {
      LOG(WARNING) << "meta len error";
      return false;
    }
    std::string buffer;
    proto::SourceMeta source_meta;
    if (!coded_input_stream.ReadString(&buffer, meta_len) || !source_meta.ParseFromString(buffer)) {
      LOG(WARNING) << "meta parse error " << meta_len;
      return false;
    }
    this->dim_ = source_meta.dim();
    this->length_ = source_meta.length();
  }

  this->labels_.reserve(this->length_);
  this->matrix_.reserve(this->length_ * this->dim_);
  for (int i=0;i<length_; ++i) {
    uint64_t len{0};
    if (!coded_input_stream.ReadVarint64(&len) || len == 0) {
      LOG(WARNING) << "read len error";
      break;
    }
    std::string buffer;
    proto::Source source;
    if (!coded_input_stream.ReadString(&buffer, len)) {
      LOG(WARNING) << "read data error, size: " << len;
      continue;
    }
    if (!source.ParseFromString(buffer)) {
      LOG(WARNING) << "parse buffer error";
      continue;
    }
    if (source.vec_size() != this->dim_) {
      LOG(WARNING) << "source dim error: <" << source.vec_size() << "," << dim_ << ">";
      continue;
    }

    this->labels_.push_back(source.label());
    this->matrix_.insert(matrix_.end(), source.vec().begin(), source.vec().end());
  }
  this->length_ = this->labels_.size();
  LOG(INFO) << "read from " << path << "get " << this->length_ << " with dim " << this->dim_;

  return true;
}*/

bool BuildTask::ReadBinary(const std::string &path) {
  std::ifstream reader(path, std::ios::binary);
  if (!reader.is_open()) {
    LOG(WARNING) << "open " << path << " error";
    return false;
  }
  std::string buffer;
  buffer.resize(1024*1024);
  uint64_t len{0};
  {
    reader.read((char*)&len, sizeof(uint64_t));
    if (len == 0) {
      LOG(WARNING) << "meta len error";
      return false;
    }
    reader.read(buffer.data(), len);
    proto::SourceMeta source_meta;
    if (!source_meta.ParseFromArray(buffer.c_str(), len)) {
      LOG(WARNING) << "meta parse error ,size: " << len;
      return false;
    }
    this->dim_ = source_meta.dim();
    this->length_ = source_meta.length();
  }
  this->labels_.reserve(this->length_);
  this->matrix_.reserve(this->length_ * this->dim_);

  for (int i=0;i<length_ && !reader.eof(); ++i) {
    reader.read((char*)&len, sizeof(uint64_t));
    if (len == 0) {
      break;
    }
    buffer.clear();
    reader.read(buffer.data(), len);

    proto::Source source;
    if (!source.ParseFromArray(buffer.c_str(), len)) {
      LOG(WARNING) << "parse buffer error";
      continue;
    }
    if (source.vec_size() != this->dim_) {
      LOG(WARNING) << "source dim error: <" << source.vec_size() << "," << dim_ << ">";
      continue;
    }

    this->labels_.push_back(source.label());
    this->matrix_.insert(matrix_.end(), source.vec().begin(), source.vec().end());
  }
  this->length_ = this->labels_.size();
  LOG(INFO) << "read from " << path << " get " << this->length_ << " with dim " << this->dim_;

  return true;
}

bool BuildTask::WriteIds(const std::string &path) {
  std::ofstream writer(path);
  if (!writer.is_open()) {
    LOG(WARNING) << "open " << path << " error";
    return false;
  }
  for (const auto& label : this->labels_) {
    writer << label << "\n";
  }
  writer.close();
  return true;
}


bool BuildTask::WriteMultiIndex(const std::string &model_dir, const std::vector<int>& index_types) {
  for (const auto& number : index_types) {
    if (!proto::Constants::IndexType_IsValid(number)) {
      continue;
    }
    auto index_type = static_cast<proto::Constants::IndexType>(number);
    auto type_it = this->type_to_keys_.find(index_type);
    if (type_it == type_to_keys_.end()) {
      continue;
    }
    std::unique_ptr<faiss::Index> index;
    try {
      index.reset(faiss::index_factory(dim_, type_it->second.c_str()));
    } catch (std::exception& e) {
      LOG(WARNING) << "build " << type_it->second << " error";
      return false;
    }
    if (index == nullptr) {
      LOG(WARNING) << "build " << type_it->second << " error";
      return false;
    }
    index->train(length_, matrix_.data());
    index->add(this->length_, this->matrix_.data());
    std::string file_path = absl::StrCat(model_dir, "/", GetIndexFileName(index_type));
    try {
      faiss::write_index(index.get(), file_path.c_str());
    } catch (std::exception& e) {
      LOG(WARNING) << "write " << file_path << " error " << e.what();
      return false;
    }
  }

  return true;
}
