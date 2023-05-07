#pragma once

#include "servables/servable.h"

namespace torch::serving {


class TorchServable : public IServable {
 public:
  bool Init(const std::string &path) override;
  PredictStatus PredictWithoutCheck(const std::shared_ptr<PredictContext>& predict_context) override;

  const std::string& GetLabel() override;
  const inference::ModelSpec& GetSpec() override;

 private:
  struct TorchModule;
  std::shared_ptr<TorchModule> torch_module_;
};

}



