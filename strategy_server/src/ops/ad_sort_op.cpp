#include "ad_sort_op.h"

#include <algorithm>

#include "proto/graph.pb.h"
#include "ops/ad.h"
#include "framework/handler_factory.h"

std::shared_ptr<std::any> AdSortOp::Compute(const KernelContext &context) {
  auto ad_list = context.AnyCast<AdList>(Session::AD_LIST_SCORE_MUT);
  auto new_ad_list = std::make_shared<AdList>();

  for (const auto& ad : *ad_list) {
    new_ad_list->push_back(ad);
  }
  std::sort(new_ad_list->begin(), new_ad_list->end(), [](const Ad& left, const Ad& right){
    return std::make_pair(left.score, left.id) > std::make_pair(right.score, right.id);
  });
  return std::make_shared<std::any>(new_ad_list);
}

OP_REGISTER(AdSortOp, Session::AD_SCORE_SORT);
