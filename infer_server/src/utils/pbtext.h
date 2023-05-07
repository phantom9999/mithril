#pragma once

#include <string>

namespace google::protobuf {
class Message;
}

namespace torch::serving {

bool ReadPbText(const std::string& filename, google::protobuf::Message* msg);

bool ReadPbBin(const std::string& filename, google::protobuf::Message* msg);

bool ReadPbJson(const std::string& filename, google::protobuf::Message* msg);

bool WritePbBin(const std::string& filename, const google::protobuf::Message& msg);


}
