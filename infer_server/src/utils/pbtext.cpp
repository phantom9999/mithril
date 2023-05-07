#include "pbtext.h"
#include <fstream>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

namespace torch::serving {

bool ReadPbText(const std::string& filename, google::protobuf::Message* msg){
  std::ifstream reader(filename);
  if (!reader.is_open()) {
    LOG(WARNING) << filename << " not found";
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(reader)), std::istreambuf_iterator<char>());
  reader.close();
  return google::protobuf::TextFormat::ParseFromString(content, msg);
}

bool ReadPbBin(const std::string& filename, google::protobuf::Message* msg) {
  std::ifstream reader(filename, std::ios::binary);
  if (!reader.is_open()) {
    LOG(WARNING) << filename << " not found";
    return false;
  }
  uint64_t len = 0;
  reader.read((char*)&len, sizeof(uint64_t));
  std::string buffer;
  buffer.resize(len);
  reader.read(buffer.data(), len);
  return msg->ParseFromString(buffer);
}

bool ReadPbJson(const std::string& filename, google::protobuf::Message* msg) {
  std::ifstream reader(filename);
  if (!reader.is_open()) {
    LOG(WARNING) << filename << " not found";
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(reader)), std::istreambuf_iterator<char>());
  reader.close();
  auto status = google::protobuf::util::JsonStringToMessage(content, msg);
  if (!status.ok()) {
    LOG(WARNING) << "parse " << filename << " error: " << status.message();
    return false;
  }
  return true;
}

bool WritePbBin(const std::string& filename, const google::protobuf::Message& msg) {
  std::ofstream writer(filename, std::ios::binary);
  if (!writer.is_open()) {
    LOG(WARNING) << filename << " not found";
    return false;
  }
  std::string buffer = msg.SerializeAsString();
  uint64_t len = buffer.size();
  writer.write((char*)&len, sizeof(uint64_t));
  writer.write(buffer.data(), len);
  return true;
}

}
