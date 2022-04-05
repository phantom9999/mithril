#include <gflags/gflags.h>
#include <glog/logging.h>
#include "config_manager.h"
#include "configs/server_config.pb.h"

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    // google::InitGoogleLogging(argv[0]);
    std::string file = "../conf/server";
    ServerConfig serverConfig;
    ConfigManager::parse(file, &serverConfig);
    // google::ShutdownGoogleLogging();
    return 0;
}