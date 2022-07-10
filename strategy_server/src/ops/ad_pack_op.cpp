#include "ad_pack_op.h"

#include "proto/graph.pb.h"
#include "proto/service.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"

std::shared_ptr<std::any> AdPackOp::Compute(const KernelContext &context) {
  auto ad_list = context.AnyCast<AdList>(Session::AD_SCORE_SORT);
  auto response = std::make_shared<StrategyResponse>();

  for (const auto& ad : *ad_list) {
    auto ad_info = response->add_ad_infos();
    ad_info->set_ad_id(ad.id);
    ad_info->set_score(ad.score);
  }
  return std::make_shared<std::any>(response);
}

OP_REGISTER(AdPackOp, Session::RESPONSE1);

