cmake_minimum_required(VERSION 3.18)
project(strategy_server)

set(CMAKE_CXX_STANDARD 17)

find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Taskflow REQUIRED)
find_package(absl REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)

include_directories(${Taskflow_INCLUDES})
include_directories(${protobuf_INCLUDES})
include_directories(${gRPC_INCLUDES})
include_directories(${absl_INCLUDES})
include_directories(${gflags_INCLUDES})
include_directories(${glog_INCLUDES})

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_target_property(protoc_location protobuf::protoc LOCATION)

add_library(proto proto/service.proto proto/graph.proto)
protobuf_generate(TARGET proto LANGUAGE cpp)
protobuf_generate(TARGET proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(src)

# 服务端

aux_source_directory(src/ops STRATEGY_SRC)
aux_source_directory(src/framework STRATEGY_SRC)
add_executable(strategy_server src/main.cpp ${STRATEGY_SRC})
target_link_libraries(
        strategy_server
        proto
        gRPC::grpc++
        glog::glog
        gflags
)


# 客户端
aux_source_directory(src/client SRC_CLIENT)
add_executable(client ${SRC_CLIENT})
target_link_libraries(client proto gRPC::grpc++)

