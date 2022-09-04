#include "config_loader.h"
#include <fcntl.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <glog/logging.h>

bool ConfigLoader::LoadPb(const std::string &path, google::protobuf::Message *message) {
  int file = open(path.c_str(), O_RDONLY);
  if (file < 0) {
    LOG(WARNING) << "open " << path << " fail";
    return false;
  }
  google::protobuf::io::FileInputStream reader{file};
  reader.SetCloseOnDelete(true);
  if (!google::protobuf::TextFormat::Parse(&reader, message)) {
    LOG(WARNING) << "parse " << path << " error";
  }
  LOG(INFO) << path << " get config: " << message->ShortDebugString();

  return true;
}
