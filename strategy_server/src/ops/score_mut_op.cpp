#include "score_mut_op.h"

#include "proto/graph.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"

std::shared_ptr<std::any> ScoreMutOp::Compute(const KernelContext &context) {
  auto bid_ad_list = context.AnyCast<AdList>(Session::AD_LIST_SCORE_ADD_BID);
  auto pos_ad_list = context.AnyCast<AdList>(Session::AD_LIST_SCORE_ADD_POS);

  std::unordered_map<uint64_t, float> bid_map;
  for (const auto& ad : *bid_ad_list) {
    bid_map.insert({ad.id, ad.score});
  }

  auto new_ad_list = std::make_shared<AdList>();
  for (const auto& pos_ad : *pos_ad_list) {
    float bid_score = bid_map[pos_ad.id];
    Ad new_ad{};
    new_ad.id = pos_ad.id;
    new_ad.score = pos_ad.score * bid_score;
    new_ad_list->push_back(new_ad);
  }

  return std::make_shared<std::any>(new_ad_list);
}

OP_REGISTER(ScoreMutOp, Session::AD_LIST_SCORE_MUT);

