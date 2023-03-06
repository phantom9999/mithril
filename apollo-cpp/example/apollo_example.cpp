#include <iostream>
#include <memory>
#include <unordered_map>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <brpc/server.h>

#include "apollo/apollo_all.h"
#include "apollo_header.h"
#include "data.pb.h"

DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
                                 "read/write operations during the last `idle_timeout_s'");

void PbPrint(const boost::shared_ptr<google::protobuf::Message>& msg) {
  if (msg == nullptr) {
    return;
  }
  std::cout << msg->GetTypeName() << " : " << msg->ShortDebugString() << std::endl;
  std::cout << "\n========\n";
}

void OtherPrint(const std::string& ns, const boost::shared_ptr<std::string>& data) {
  if (data == nullptr) {
    return;
  }
  std::cout << ns << " : " << *data << std::endl;
  std::cout << "\n========\n";
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (!apollo::ApolloConfig::Get().Init()) {
    std::cout << "init error\n";
    return -1;
  }
  brpc::Server server;
  apollo::ApolloDeliverServiceImpl apollo_deliver_service;
  if (server.AddService(&apollo_deliver_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(WARNING) << "register service error";
    return -1;
  }
  // Start the server.
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start server";
    return -1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(10));

  PbPrint(apollo::GetProp1Key1());
  PbPrint(apollo::GetProp1Key2());
  PbPrint(apollo::GetProp2Key3());
  PbPrint(apollo::GetMsg<example::Msg1>("Prop1", "Key2"));
  OtherPrint(apollo::kOther1, apollo::GetOther(apollo::kOther1));

  server.RunUntilAskedToQuit();
  apollo::ApolloConfig::Get().Close();
  return 0;
}
