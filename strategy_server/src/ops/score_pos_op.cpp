#include "score_pos_op.h"

#include <unordered_map>

#include "proto/service.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"

std::shared_ptr<std::any> ScorePosOp::Compute(const KernelContext &context) {
  auto request = context.AnyCast<StrategyRequest>(Session::REQUEST1);
  auto ad_list = context.AnyCast<AdList>(Session::AD_LIST_SCORE_INIT);

  std::unordered_map<uint64_t, size_t> ad_pos;
  for (size_t index = 0; index < request->ad_infos_size(); ++index) {
    ad_pos.insert({request->ad_infos(index).ad_id(), index});
  }
  auto new_ad_list = std::make_shared<AdList>();
  for (const auto& ad : *ad_list) {
    float score = ad.score;
    size_t index = ad_pos[ad.id];
    score *= (ad_pos.size() + 0.1 - index) / ad_pos.size();
    Ad new_ad{};
    new_ad.id = ad.id;
    new_ad.score = score;
    new_ad_list->push_back(new_ad);
  }

  return std::make_shared<std::any>(new_ad_list);
}

OP_REGISTER(ScorePosOp, Session::AD_LIST_SCORE_ADD_POS);
