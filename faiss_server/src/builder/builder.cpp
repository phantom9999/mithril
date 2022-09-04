#include <vector>
#include <fstream>

#include <gflags/gflags.h>

#include "builder_config.pb.h"

#include "builder_flags.h"
#include "common/config_loader.h"
#include "build_task.h"

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  proto::BuilderConfig builder_config;
  if (!ConfigLoader::LoadPb(FLAGS_conf, &builder_config)) {
    return 0;
  }

  for (const auto& sub_config : builder_config.tasks()) {
    BuildTask task{sub_config, builder_config.output_path()};
    task.BuildIndex();
  }
  return 0;
}
