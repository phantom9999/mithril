#include <memory>
#include <any>
#include <utility>
#include <vector>
#include <string>

#include <grpc++/grpc++.h>
#include "proto/service.pb.h"
#include "proto/graph.pb.h"
#include "proto/service.grpc.pb.h"
#include "framework/processor.h"
#include "framework/timer.h"


class StrategyServiceImpl : public StrategyService::Service {
public:
  explicit StrategyServiceImpl(std::shared_ptr<Processor>  processor) : processor_(std::move(processor)) { }
  ::grpc::Status Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                      ::StrategyResponse *response) override {
    std::string msg = "logid: " + std::to_string(request->logid());
    Timer timer(msg);
    if (processor_->Run(request, response)) {
      return grpc::Status::OK;
    }
    return grpc::Status::CANCELLED;
  }

private:
  std::shared_ptr<Processor> processor_;
};

void BuildGraph(GraphsConf* conf) {
  auto graph_def = conf->add_graph_defs();
  graph_def->set_name("ad_process");
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_init");
    op_def->add_inputs(Session::REQUEST1);
    op_def->set_output(Session::AD_LIST_SCORE_INIT);
  }
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_bid");
    op_def->add_inputs(Session::AD_LIST_SCORE_INIT);
    op_def->add_inputs(Session::REQUEST1);
    op_def->set_output(Session::AD_LIST_SCORE_ADD_BID);
  }
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_pos");
    op_def->add_inputs(Session::AD_LIST_SCORE_INIT);
    op_def->add_inputs(Session::REQUEST1);
    op_def->set_output(Session::AD_LIST_SCORE_ADD_POS);
  }
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_mut");
    op_def->add_inputs(Session::AD_LIST_SCORE_ADD_POS);
    op_def->add_inputs(Session::AD_LIST_SCORE_ADD_BID);
    op_def->set_output(Session::AD_LIST_SCORE_MUT);
  }
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_sort");
    op_def->add_inputs(Session::AD_LIST_SCORE_MUT);
    op_def->set_output(Session::AD_SCORE_SORT);
  }
  {
    auto op_def = graph_def->add_op_defs();
    op_def->set_name("ad_pack");
    op_def->add_inputs(Session::AD_SCORE_SORT);
    op_def->set_output(Session::RESPONSE1);
  }
}


int main() {
  GraphsConf graphs_conf;
  BuildGraph(&graphs_conf);
  auto processor = std::make_shared<Processor>(graphs_conf);
  processor->DumpGraph(std::cout);
  std::string server_address("0.0.0.0:8000");
  StrategyServiceImpl service{processor};

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
  server->Wait();
}



