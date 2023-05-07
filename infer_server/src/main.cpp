#include <string>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <absl/cleanup/cleanup.h>
#include <absl/strings/str_format.h>
#include "service/torch_server.h"
#include "version.h"

int main(int argc, char** argv) {
  {
    google::SetVersionString(
        absl::StrFormat("%s:%s", torch::serving::branch_name.c_str(), torch::serving::commit_hash.c_str()));
  }
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  auto cleanup = absl::MakeCleanup([](){
    google::ShutdownGoogleLogging();
    google::ShutDownCommandLineFlags();
  });
  torch::serving::TorchServer torch_server;
  if (!torch_server.Init()) {
    return -1;
  }
  torch_server.WaitUntilStop();

  return 0;
}

