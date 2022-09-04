#include "server/service_impl.h"
#include <faiss/index_io.h>
#include <faiss/Index.h>
#include <glog/logging.h>

#include "index_manager.h"
#include "server/search_param.h"
#include "common/timer.h"

grpc::Status ServiceImpl::Retrieval(::grpc::ServerContext *context,
                                    const ::proto::RetrievalRequest *request,
                                    ::proto::RetrievalResponse *response) {
  SearchParam param{};
  param.model_name = request->model_name();
  param.topk = request->topk();
  param.query_size = request->query_size();
  param.vec = request->query_vec().data();
  param.vec_size = request->query_vec_size();

  Timer timer;
  SearchResult result{};
  auto status = this->index_manager_->Search(param, &result);
  if (status != SearchStatus::OK) {
    return {grpc::INTERNAL, ToString(status)};
  }
  auto recall_cost = timer.UsCost();

  for (int i=0;i<result.batch_size;++i) {
    auto* batch = response->add_batches();
    batch->set_id(i);
    for (int j=0;j<result.size_per_batch;++j) {
      uint32_t index = i * result.size_per_batch + j;
      auto item = batch->add_items();
      item->set_label(result.labels[index]);
      item->set_score(result.scores[index]);
    }
  }
  response->set_version(result.version);
  auto res_cost = timer.UsCost();

  std::string model_name = proto::Constants::ModelName_Name(request->model_name());

  LOG(INFO) << "model: " << model_name << " query_size: " << request->query_size() << "; topk: " << request->topk()
    << "; total cost " << res_cost << "us; recall cost " << recall_cost << "us";

  return grpc::Status::OK;
}
grpc::Status ServiceImpl::Status(::grpc::ServerContext *context,
                                 const ::proto::StatusRequest *request,
                                 ::proto::StatusResponse *response) {
  using proto::Constants;
  for (int i=Constants::ModelName_MIN;i<Constants::ModelName_ARRAYSIZE; ++i) {
    std::vector<uint64_t> versions;
    if (!Constants::ModelName_IsValid(i)) {
      continue;
    }
    auto model_name = static_cast<Constants::ModelName>(i);
    std::vector<std::tuple<proto::Constants::IndexType, uint64_t, uint64_t>> status;
    uint64_t version;
    if (!index_manager_->GetStatus(model_name, &version, &status) || status.empty()) {
      continue;
    }
    auto model_status = response->add_model_status();
    model_status->set_model_name(model_name);
    model_status->set_version(version);
    for (const auto& item : status) {
      auto* index_status = model_status->add_index_status();
      index_status->set_index_type(std::get<0>(item));
      index_status->set_length(std::get<1>(item));
      index_status->set_dim(std::get<2>(item));
    }
  }
  return grpc::Status::OK;
}

ServiceImpl::ServiceImpl(IndexManager *index_manager) : index_manager_{index_manager} {

}
