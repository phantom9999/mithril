#include "feature_service_impl.h"
#include <sw/redis++/async_redis++.h>


namespace ad {

FeatureServiceImpl::~FeatureServiceImpl() {

}
grpc::Status FeatureServiceImpl::GetFeature(::grpc::ServerContext *context,
                                            const ::proto::FeatureRequest *request,
                                            ::proto::FeatureResponse *response) {
  response->set_user_id(request->user_id());
  response->set_biz(request->biz());
  response->set_trace_id(request->trace_id());

  std::vector<sw::redis::Future<sw::redis::Optional<std::string>>> fu_list;
  fu_list.reserve(request->names_size());
  proto::FeatureKey feature_key;
  feature_key.set_biz(request->biz());
  for (const auto& item : request->names()) {
    feature_key.set_name(static_cast<proto::Define_FeatureName>(item));
    feature_key.set_user_id(request->user_id());
    fu_list.push_back(client_->get(feature_key.SerializeAsString()));
  }

  for (auto&& fu : fu_list) {
    std::string value;
    try {
      auto result = fu.get();
      if (!result.has_value() || result->empty()) {
        continue;
      }
      value = std::move(result.value());
    } catch (...) {
      continue;
    }
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
  return grpc::Status::OK;
}
FeatureServiceImpl::FeatureServiceImpl(const RedisClientPtr &redis_client_ptr) : client_(redis_client_ptr) {

}
}

