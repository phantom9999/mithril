#include <iostream>
#include <memory>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include "configs/server_config.pb.h"


std::shared_ptr<ServerConfig> readConfig(const std::string& path) {
    int file = open(path.c_str(), O_RDONLY);
    if (file < 0) {
        std::cout << "open " << path << " fail\n";
        return nullptr;
    }
    google::protobuf::io::FileInputStream reader{file};
    reader.SetCloseOnDelete(true);
    auto config = std::make_shared<ServerConfig>();
    if (!google::protobuf::TextFormat::Parse(&reader, config.get())) {
        std::cout << "parse " << path << " error\n";
        return nullptr;
    }
    return config;
}



int main(int argc, char** argv) {
    std::string file = "../conf/server.conf";
    auto config = readConfig(file);
    if (config != nullptr) {
        std::cout << config->DebugString();
    }

    return 0;
}