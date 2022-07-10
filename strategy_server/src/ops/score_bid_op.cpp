#include "score_bid_op.h"

#include <unordered_map>

#include "proto/graph.pb.h"
#include "proto/service.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"

std::shared_ptr<std::any> ScoreBidOp::Compute(const KernelContext &context) {
  auto request = context.AnyCast<StrategyRequest>(Session::REQUEST1);
  auto ad_list = context.AnyCast<AdList>(Session::AD_LIST_SCORE_INIT);
  auto new_ad_list = std::make_shared<AdList>();

  std::unordered_map<uint64_t, float> ad_bids;
  for (const auto& ad : request->ad_infos()) {
    ad_bids.insert({ad.ad_id(), ad.bid()});
  }

  for (const auto& ad : *ad_list) {
    float bid = ad_bids[ad.id];
    Ad new_ad{};
    new_ad.id = ad.id;
    new_ad.score = ad.score * bid;
    new_ad_list->push_back(new_ad);
  }
  return std::make_shared<std::any>(new_ad_list);
}

OP_REGISTER(ScoreBidOp, Session::AD_LIST_SCORE_ADD_BID);
