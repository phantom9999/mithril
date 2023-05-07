#define BOOST_TEST_MODULE torch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "server_config.pb.h"
#include "model/model_manager.h"

BOOST_AUTO_TEST_CASE(model_mananger) {
  torch::serving::ModelManagerConfig config;
  auto* model_config = config.add_mode_configs();
  model_config->set_name("MODEL_1");
  model_config->set_path("../data/model_1");
  torch::serving::ModelManager model_manager;
  BOOST_ASSERT(model_manager.Init(config));
  auto servable = model_manager.GetServableByVersion("MODEL_1");
  BOOST_ASSERT(servable != nullptr);
  model_manager.Stop();
}

