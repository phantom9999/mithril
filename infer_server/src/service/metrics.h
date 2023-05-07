#pragma once

#include <memory>

namespace prometheus {
class Registry;

template<typename TT>
class Family;

class Summary;
}


namespace torch::serving {
class MetricsConfig;

class Metrics {
 public:
  Metrics(const std::shared_ptr<prometheus::Registry>& registry, const MetricsConfig& config);
  explicit Metrics(const std::shared_ptr<prometheus::Registry>& registry);

  prometheus::Summary* GetModelSummary(const std::string& model);
  prometheus::Summary* GetServiceSummary(const std::string& service);

 private:
  const std::shared_ptr<prometheus::Registry> registry_;
  prometheus::Family<prometheus::Summary>& model_family_;
  prometheus::Family<prometheus::Summary>& service_family_;
  const uint32_t windows_;
  std::shared_ptr<MetricsConfig> config_{nullptr};
};
}
