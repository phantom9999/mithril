#include "feature_service_impl.h"

#include "resource_manager.h"
#include "proto/sink.pb.h"


namespace ad {

grpc::Status FeatureServiceImpl::GetFeature(::grpc::ServerContext *context,
                                            const ::proto::FeatureRequest *request,
                                            ::proto::FeatureResponse *response) {
  response->set_user_id(request->user_id());
  response->set_biz(request->biz());
  response->set_trace_id(request->trace_id());

  std::vector<std::string> keys;
  keys.reserve(request->names_size());
  std::vector<std::string> values;

  proto::FeatureKey feature_key;
  feature_key.set_biz(request->biz());
  for (const auto& item : request->names()) {
    feature_key.set_name(static_cast<proto::Define_FeatureName>(item));
    feature_key.set_user_id(request->user_id());
    keys.push_back(feature_key.SerializeAsString());
  }

  resource_manager_->MGet(keys, &values);

  for (const auto& value: values) {
    proto::FeatureValue feature_value;
    if (!feature_value.ParseFromString(value)) {
      continue;
    }
    switch (feature_value.type_case()) {
      case proto::FeatureValue::kFeatureItem: {
        auto item = response->add_items();
        item->Swap(feature_value.mutable_feature_item());
        break;
      }
      case proto::FeatureValue::kFeatureGroup: {
        auto* group = feature_value.mutable_feature_group();
        for (int i=0;i<group->items_size();++i) {
          response->add_items()->Swap(group->mutable_items(i));
        }
        break;
      }
      default:break;
    }
  }
  Sink(*response);

  return grpc::Status::OK;
}
FeatureServiceImpl::FeatureServiceImpl(const std::shared_ptr<ResourceManager>& resource_manager)
  : resource_manager_(resource_manager) {

}
void FeatureServiceImpl::Sink(const proto::FeatureResponse &response) {
  if (response.items_size() == 0) {
    return;
  }
  proto::FeatureLog feature_log;
  feature_log.set_user_id(response.user_id());
  feature_log.set_trace_id(response.trace_id());
  feature_log.set_biz(response.biz());
  for (const auto& item : response.items()) {
    auto meta = feature_log.add_metas();
    meta->set_name(item.name());
    meta->set_version(item.version());
  }
  auto* host = feature_log.mutable_host_meta();
  host->set_host(ResourceManager::GetHost());
  host->set_timestamp(std::time(nullptr));
  resource_manager_->Sink(feature_log.SerializeAsString());
}
}

