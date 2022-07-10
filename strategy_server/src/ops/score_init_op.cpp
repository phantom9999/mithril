#include "score_init_op.h"
#include "proto/service.pb.h"
#include "proto/graph.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"
#include <glog/logging.h>

std::shared_ptr<std::any> ScoreInitOp::Compute(const KernelContext &context) {
  auto request = context.AnyCast<StrategyRequest>(Session_Type_REQUEST1);
  auto ad_list = std::make_shared<AdList>();
  for (const auto& ad_info : request->ad_infos()) {
    Ad ad{};
    ad.id = ad_info.ad_id();
    ad.score = 1.0;
    ad_list->emplace_back(ad);
  }
  return std::make_shared<std::any>(ad_list);
}

OP_REGISTER(ScoreInitOp, Session::AD_LIST_SCORE_INIT);

