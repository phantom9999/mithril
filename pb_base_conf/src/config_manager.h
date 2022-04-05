#pragma once

#include <string>

namespace google::protobuf {
class Message;
}

class ConfigManager {
public:
  static bool parse(const std::string& path, google::protobuf::Message* message);
  ConfigManager() = delete;
  ~ConfigManager() = delete;
};
