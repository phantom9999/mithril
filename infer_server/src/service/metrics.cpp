#include "metrics.h"
#include <prometheus/registry.h>
#include <prometheus/summary.h>

#include "server_config.pb.h"

namespace {
const prometheus::Summary::Quantiles& GetQuantiles(const std::shared_ptr<torch::serving::MetricsConfig>& config) {
  static prometheus::Summary::Quantiles quantiles;
  static std::once_flag flag;
  std::call_once(flag, [&]() {
    if (config != nullptr && config->quantiles_size() > 0) {
      quantiles.reserve(config->quantiles_size());
      for (const auto& entry : config->quantiles()) {
        if (entry.quantile_percent() <= 0 || entry.error_percent() <= 0) {
          continue;
        }
        quantiles.emplace_back(entry.quantile_percent(), entry.error_percent());
      }
    }
    if (quantiles.empty()) {
      quantiles = {{0.5, 0.01}, {0.99, 0.001}, {0.9999, 0.001}};
    }
  });
  return quantiles;
}
}


namespace torch::serving {

Metrics::Metrics(const std::shared_ptr<prometheus::Registry> &registry, const MetricsConfig& config)
  : registry_(registry),
    model_family_(prometheus::BuildSummary()
      .Name("model_latency")
      .Help("no thing")
      .Register(*registry)),
    service_family_(prometheus::BuildSummary()
      .Name("service_latency")
      .Help("no thing")
      .Register(*registry)),
    windows_(config.time_windows()) {
  config_ = std::make_shared<MetricsConfig>(config);
}

Metrics::Metrics(const std::shared_ptr<prometheus::Registry> &registry)
  : registry_(registry),
    model_family_(prometheus::BuildSummary()
                      .Name("model_latency")
                      .Help("no thing")
                      .Register(*registry)),
    service_family_(prometheus::BuildSummary()
                        .Name("service_latency")
                        .Help("no thing")
                        .Register(*registry)),
    windows_(60) {

}

prometheus::Summary *Metrics::GetModelSummary(const std::string &model) {
  return &model_family_.Add({{"model", model}}, GetQuantiles(config_), std::chrono::seconds{windows_});
}
prometheus::Summary *Metrics::GetServiceSummary(const std::string &service) {
  return &service_family_.Add({{"service", service}}, GetQuantiles(config_), std::chrono::seconds{windows_});
}

}


