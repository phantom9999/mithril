#include "shared_batch_scheduler.h"

#include <absl/time/time.h>
#include <absl/time/clock.h>
#include <absl/types/span.h>

#include "batch_config.pb.h"
#include "model_spec.pb.h"
#include "kserve_predict_v2.pb.h"

#include "model/predict_context.h"
#include "servables/servable.h"
#include "servables/torch_servable.h"

namespace torch::serving {

void MergeRequest(const std::vector<const inference::ModelInferRequest*>& request_list, const inference::ModelSpec& spec, inference::ModelInferRequest* request) {
  if (request_list.empty()) {
    return;
  }
  auto& first = request_list[0];
  request->set_id(first->id());
  request->set_model_name(first->model_name());
  std::unordered_map<std::string, std::vector<const inference::ModelInferRequest::InferInputTensor*>> tensor_map;
  for (const auto& sub_request : request_list) {
    for (const auto& input : sub_request->inputs()) {
      tensor_map[input.name()].push_back(&input);
    }
  }
  for (const auto& feature_spec : spec.feature_specs()) {
    auto* merge_proto = request->add_inputs();
    auto& proto_list = tensor_map[feature_spec.name()];
    size_t shape_size = 0;
    size_t item_size = 0;

    for (const auto& proto : proto_list) {
      size_t current = 1;
      for (const auto& dim : proto->shape()) {
        current *= dim;
      }
      shape_size += current;
      item_size += proto->shape(0);
    }
    auto* content = merge_proto->mutable_contents();
    switch (feature_spec.dtype()) {
      case inference::DT_FLOAT: {
        content->mutable_fp32_contents()->Reserve(shape_size);
        break;
      }
      case inference::DT_DOUBLE: {
        content->mutable_fp64_contents()->Reserve(shape_size);
        break;
      }
      case inference::DT_INT32: {
        content->mutable_int_contents()->Reserve(shape_size);
        break;
      }
      case inference::DT_UINT32: {
        content->mutable_uint_contents()->Reserve(shape_size);
        break;
      }
      case inference::DT_INT64: {
        content->mutable_int64_contents()->Reserve(shape_size);
        break;
      }
      case inference::DT_UINT64: {
        content->mutable_uint64_contents()->Reserve(shape_size);
        break;
      }
      default: {
        break;
      }
    }
    for (const auto& proto : proto_list) {
      auto& sub_content = proto->contents();
      switch (feature_spec.dtype()) {
        case inference::DT_FLOAT: {
          content->mutable_fp32_contents()->Add(sub_content.fp32_contents().begin(), sub_content.fp32_contents().end());
          break;
        }
        case inference::DT_DOUBLE: {
          content->mutable_fp64_contents()->Add(sub_content.fp64_contents().begin(), sub_content.fp64_contents().end());
          break;
        }
        case inference::DT_INT32: {
          content->mutable_int_contents()->Add(sub_content.int_contents().begin(), sub_content.int_contents().end());
          break;
        }
        case inference::DT_UINT32: {
          content->mutable_uint_contents()->Add(sub_content.uint_contents().begin(), sub_content.uint_contents().end());
          break;
        }
        case inference::DT_INT64: {
          content->mutable_int64_contents()->Add(sub_content.int64_contents().begin(), sub_content.int64_contents().end());
          break;
        }
        case inference::DT_UINT64: {
          content->mutable_uint64_contents()->Add(sub_content.uint64_contents().begin(), sub_content.uint64_contents().end());
          break;
        }
        default: {
          break;
        }
      }
    }
    merge_proto->set_datatype(feature_spec.dtype());
    merge_proto->add_shape(item_size);
    merge_proto->mutable_shape()->Add(feature_spec.shape().begin(), feature_spec.shape().end());
  }
}

void SplitResponse(const inference::ModelInferResponse& response, const std::vector<inference::ModelInferResponse*>& response_list, const std::vector<size_t>& item_size_list) {
  for (const auto& merge_proto : response.outputs()) {
    if (merge_proto.shape_size() == 0) {
      continue;
    }

    size_t shape_per_item = 1;
    for (int i=1;i<merge_proto.shape_size();++i) {
      shape_per_item *= merge_proto.shape(i);
    }
    size_t data_index = 0;
    for (size_t i=0; i<response_list.size(); ++i) {
      auto& sub_response = response_list[i];
      const auto& item_size = item_size_list[i];
      auto* proto = sub_response->add_outputs();
      proto->set_name(merge_proto.name());
      size_t item_data_size = item_size * shape_per_item;
      auto& merge_content = merge_proto.contents();
      auto* sub_content = proto->mutable_contents();
      switch (merge_proto.datatype()) {
        case inference::DT_FLOAT: {
          absl::Span<const float> data(merge_content.fp32_contents().data() + data_index, item_data_size);
          sub_content->mutable_fp32_contents()->Add(data.begin(), data.end());
          break;
        }
        case inference::DT_DOUBLE: {
          absl::Span<const double> data(merge_content.fp64_contents().data() + data_index, item_data_size);
          sub_content->mutable_fp64_contents()->Add(data.begin(), data.end());
          break;
        }
        case inference::DT_INT32: {
          absl::Span<const int32_t> data(merge_content.int_contents().data() + data_index, item_data_size);
          sub_content->mutable_int_contents()->Add(data.begin(), data.end());
          break;
        }
        case inference::DT_UINT32: {
          absl::Span<const uint32_t> data(merge_content.uint_contents().data() + data_index, item_data_size);
          sub_content->mutable_uint_contents()->Add(data.begin(), data.end());
          break;
        }
        case inference::DT_INT64: {
          absl::Span<const int64_t> data(merge_content.int64_contents().data() + data_index, item_data_size);
          sub_content->mutable_int64_contents()->Add(data.begin(), data.end());
          break;
        }
        case inference::DT_UINT64: {
          absl::Span<const uint64_t> data(merge_content.uint64_contents().data() + data_index, item_data_size);
          sub_content->mutable_uint64_contents()->Add(data.begin(), data.end());
          break;
        }
        default: {
          break;
        }
      }
      data_index += item_data_size;
    }
  }
}

void ServableQueue::AddTask(const BatchTaskPtr &task) {
  std::unique_lock lock(mutex_);
  if (group_list_.empty()) {
    group_list_.push_back(std::make_shared<BatchTaskGroup>());
    group_list_.back()->task_list_.reserve(queue_size_);
  }
  if (IsGroupClose(group_list_.back())) {
    group_list_.push_back(std::make_shared<BatchTaskGroup>());
    group_list_.back()->task_list_.reserve(queue_size_);
  }
  auto& group = group_list_.back();
  group->task_list_.push_back(task);
  group->item_size_ += task->size;
}
BatchTaskGroupPtr ServableQueue::GetTaskGroup() {
  std::unique_lock lock(mutex_);
  if (group_list_.empty()) {
    return nullptr;
  }
  auto group = group_list_.front();
  if (IsGroupClose(group)) {
    group_list_.pop_front();
    return group;
  }

  return nullptr;
}
bool ServableQueue::IsGroupClose(const BatchTaskGroupPtr &group) const {
  if (group->task_list_.size() >= queue_size_) {
    return true;
  }
  if (group->item_size_ > size_windows_) {
    return true;
  }
  if (group->item_size_ == 0) {
    return false;
  }

  return absl::ToInt64Milliseconds(absl::Now() - group->begin_) >= time_windows_;
}
ServableQueue::ServableQueue(uint32_t time_windows, uint32_t size_windows, uint32_t queue_size)
  : time_windows_(time_windows), size_windows_(size_windows), queue_size_(queue_size) {

}

void SharedBatchScheduler::Work() {
  BatchTaskGroupPtr group_ptr;
  {
    std::unique_lock lock(mutex_);
    size_t size = queue_.size();
    for (int i=0;i<size;++i) {
      if (queue_.empty()) {
        break;
      }
      auto& node = *current_;
      auto before = current_;
      ++current_;
      if (current_ == queue_.end()) {
        current_ = queue_.begin();
      }
      if (node->IsStop()) {
        queue_.erase(before);
        continue;
      }
      group_ptr = node->GetTaskGroup();
      if (group_ptr != nullptr) {
        break;
      }
    }
  }
  if (group_ptr == nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }
  auto servable = group_ptr->task_list_.front()->servable;
  auto& spec = servable->GetSpec();

  std::vector<const inference::ModelInferRequest*> request_list;
  std::vector<inference::ModelInferResponse*> response_list;
  std::vector<size_t> item_size_list;

  inference::ModelInferRequest request;
  inference::ModelInferResponse response;
  for (const auto& item : group_ptr->task_list_) {
    request_list.push_back(item->context->request_);
    item_size_list.push_back(item->size);
  }
  MergeRequest(request_list, spec, &request);
  auto merge_context = std::make_shared<PredictContext>(&request, &response);
  auto status = servable->Predict(merge_context);
  SplitResponse(response, response_list, item_size_list);

  auto& merge_state = merge_context->time_state_;
  for (const auto& item : group_ptr->task_list_) {
    auto& state = item->context->time_state_;
    state.before_pack = merge_state.before_pack;
    state.before_predict = merge_state.before_predict;
    state.before_unpack = merge_state.before_unpack;
    state.after_unpack = merge_state.after_unpack;

    item->promise.set_value(status);
  }
}

SharedBatchScheduler::SharedBatchScheduler(const BatchConfig& config) {
  uint32_t num = config.num_batch_threads() <= 0 ? std::thread::hardware_concurrency() : config.num_batch_threads();
  size_windows_ = config.max_batch_size() <= 0 ? 1 : config.max_batch_size();
  time_windows_ = config.batch_timeout_micros() <= 0 ? 1: config.batch_timeout_micros();
  queue_size_ = config.max_enqueued_batches() <= 0 ? UINT32_MAX : config.max_enqueued_batches();

  current_ = queue_.begin();
  workers_.reserve(num);
  for (int i=0;i<num;++i) {
    workers_.emplace_back([this](){
      while (running_) {
        Work();
      }
    });
  }
}
SharedBatchScheduler::~SharedBatchScheduler() {
  running_ = false;
  for (auto& item : workers_) {
    item.join();
  }
}
ServableQueueNode SharedBatchScheduler::AddQueue() {
  std::unique_lock lock(mutex_);
  queue_.push_front(std::make_shared<ServableQueue>(time_windows_, size_windows_, queue_size_));
  current_ = queue_.begin();
  return queue_.begin();
}
}
