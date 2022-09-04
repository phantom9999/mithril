#pragma once

#include <string>

namespace google::protobuf {
class Message;
}

class ConfigLoader {
 public:
  static bool LoadPb(const std::string& path, google::protobuf::Message* message);
};

