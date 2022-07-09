// #include "service_impl.h"
// #include <grpc++/grpc++.h>
#include <memory>
#include <any>
#include <utility>
#include <vector>
#include <string>
#include <utility>
#include <mutex>
#include <queue>
#include <taskflow/taskflow.hpp>
#include "proto/service.pb.h"
#include "proto/graph.pb.h"

class KernelContext {
 public:
  KernelContext() {
    data_.resize(Session_Type_Type_MAX);
  }

  void Put(Session::Type key, const std::shared_ptr<std::any>& value) {
    data_[key] = value;
  }

  bool BuildSubContext(KernelContext* context, const std::vector<Session::Type>& inputs) {
    for (const auto& key : inputs) {
      context->Put(key, data_[key]);
    }
  }

  template<typename Type>
  std::shared_ptr<Type> AnyCast(Session_Type index) {
    auto data = data_[index];
    if (data == nullptr) {
      return nullptr;
    }
    try {
      return std::any_cast<std::shared_ptr<Type>>(*data);
    } catch (std::bad_any_cast const & e) {
      return nullptr;
    }
  }

  void Clear() {
    data_.clear();
    data_.resize(Session_Type_Type_MAX);
  }

 private:
  std::vector<std::shared_ptr<std::any>> data_;
};

class OpKernal {
 public:
  OpKernal(const std::vector<Session::Type>& inputs, Session::Type output) : inputs_(inputs), output_(output) {

  }

  void BindTask(const tf::Task& task) {
    task_ = task;
  }

  void Run() {
    auto* global_context = static_cast<KernelContext *>(task_.data());
    KernelContext context;
    global_context->BuildSubContext(&context, inputs_);
    auto result = Compute(context);
    global_context->Put(output_, result);
  }

 protected:
  virtual std::shared_ptr<std::any> Compute(const KernelContext& context) = 0;

 private:
  tf::Task task_;
  std::vector<Session::Type> inputs_;
  Session::Type output_;
};

class Creater {
 public:
  virtual OpKernal* Create(const std::vector<Session::Type>& inputs, Session::Type output) = 0;
};

template<typename Op>
class OpCreater : public Creater {
 public:
  OpKernal* Create(const std::vector<Session::Type>& inputs, Session::Type output) final {
    return new Op(inputs, output);
  }
};

class ProcessHandler {
 public:
  explicit ProcessHandler(tf::Executor& executor): executor_(executor) { }
  /**
   * 非线程安全
   * @param request
   * @param response
   * @param response_type
   * @return
   */
  bool Run(const StrategyRequest* request, Session::Type requestType, StrategyResponse* response, Session_Type response_type) {
    global_context_.Clear();
    auto requestPtr = std::make_shared<StrategyRequest>();
    requestPtr->CopyFrom(*request);
    global_context_.Put(requestType, std::make_shared<std::any>(request));
    executor_.run(taskflow_).get();
    auto responsePtr = global_context_.AnyCast<StrategyResponse>(response_type);
    if (responsePtr == nullptr) {
      return false;
    }
    response->Swap(responsePtr.get());
  }

 private:
  tf::Executor& executor_;
  tf::Taskflow taskflow_;
  KernelContext global_context_;
  std::unordered_map<Session_Type, OpKernal*> kernels_;

  friend class HandlerFactory;
};

class HandlerFactory {
 public:
  explicit HandlerFactory(const GraphDef& graph_def) {
    for (const auto& opdef : graph_def.op_defs()) {
      graph_.insert({opdef.output(), {opdef.inputs().begin(), opdef.inputs().end()}});
    }
  }

  std::unique_ptr<ProcessHandler> BuildNew(tf::Executor& executor) {
    auto handler = std::make_unique<ProcessHandler>(executor);
    auto& opmap = handler->kernels_;
    std::unordered_map<Session_Type, tf::Task> task_map;
    // 绑定内核
    for (const auto& [output, inputs] : graph_) {
      auto* kernel = creaters_[output].Create(inputs, output);
      auto task = handler->taskflow_.emplace([kernel](){
        kernel->Run();
      });
      task.data(&handler->global_context_);
      kernel->BindTask(task);
      opmap.emplace(output, kernel);
      task_map.emplace(output, task);
    }
    // 处理依赖问题
    for (const auto& [output, inputs] : graph_) {
      auto& task = task_map[output];
      for (const auto& key : inputs) {
        if (auto it = task_map.find(key); it != task_map.end()) {
          task.precede(it->second);
        }
      }
    }
    return handler;
  }

 public:
  static void Register(Session::Type key, Creater creater) {
    creaters_[key] = std::move(creater);
  }

 private:
  // <output, [input]>
  std::unordered_map<Session_Type, std::vector<Session_Type>> graph_;
  inline static std::vector<Creater> creaters_{};
};

class GraphFactory {
 public:
  explicit GraphFactory(const GraphsConf& graphs_conf) {
    for (const auto& graph_def : graphs_conf.graph_defs()) {
      factories_.emplace(graph_def.name(), std::make_unique<HandlerFactory>(graph_def));
    }
  }

  std::unique_ptr<ProcessHandler> BuildNew(const std::string& name, tf::Executor& executor) {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
      return nullptr;
    }
    return it->second->BuildNew(executor);
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<HandlerFactory>> factories_;
};

struct HandlerQueue {
  std::queue<std::unique_ptr<ProcessHandler>> queue_;
  std::mutex mutex_;
};

class Processor {
public:
  Processor(const GraphsConf& graphs_conf, size_t max_concurrent) : executor_(max_concurrent) {
    factory_ = std::make_unique<GraphFactory>(graphs_conf);
    for (const auto& graph_def : graphs_conf.graph_defs()) {
      std::string name = graph_def.name();
      auto handler_queue = std::make_unique<HandlerQueue>();
      for (size_t i=0; i<std::thread::hardware_concurrency(); ++i) {
        handler_queue->queue_.emplace(factory_->BuildNew(name, executor_));
      }
      queue_.emplace(name, handler_queue.release());
    }
  }

  bool Run(const StrategyRequest* request, StrategyResponse* response) {
    auto [name, request_type, response_type] = SelectGraph(request);
    auto it = queue_.find(name);
    if (it == queue_.end()) {
      // 没找到
      return false;
    }
    std::unique_ptr<ProcessHandler> handler;
    {
      std::lock_guard lock(it->second->mutex_);
      if (!it->second->queue_.empty()) {
        handler = std::move(it->second->queue_.front());
        it->second->queue_.pop();
      }
    }
    if (handler == nullptr) {
      handler = factory_->BuildNew(name, executor_);
    }
    bool result = handler->Run(request, request_type, response, response_type);
    {
      std::lock_guard lock(it->second->mutex_);
      it->second->queue_.push(std::move(handler));
    }
    return result;
  }

  std::tuple<std::string, Session_Type, Session_Type> SelectGraph(const StrategyRequest* request) {
    // todo: 选图的逻辑
    return std::make_tuple("xx", Session_Type_REQUEST, Session_Type_RESPONSE);
  }

private:
  std::unique_ptr<GraphFactory> factory_;
  tf::Executor executor_;
  std::unordered_map<std::string, std::unique_ptr<HandlerQueue>> queue_;
};

int main() {
  /*std::string server_address("0.0.0.0:8000");
  ServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();*/
}



