cmake_minimum_required(VERSION 3.22)
project(feature-server)

set(CMAKE_CXX_STANDARD 17)

find_package(protobuf REQUIRED)
find_package(Glog REQUIRED)
find_package(gflags REQUIRED)
find_package(gRPC REQUIRED)
find_package(absl REQUIRED)
find_package(redis++ REQUIRED)
find_package(hiredis REQUIRED)
find_package(RdKafka REQUIRED)
find_package(Boost COMPONENTS thread REQUIRED)

include_directories(${protobuf_INCLUDE_DIR})
include_directories(${gflags_INCLUDE_DIR})
include_directories(${glog_INCLUDE_DIR})
include_directories(${gRPC_INCLUDE_DIR})
include_directories(${absl_INCLUDE_DIR})
include_directories(${redis++_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_target_property(protoc_location protobuf::protoc LOCATION)

add_library(feature_protos
        proto/service.proto
        proto/feature.proto
        proto/sink.proto
        )
protobuf_generate(TARGET feature_protos LANGUAGE cpp)
protobuf_generate(TARGET feature_protos LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

aux_source_directory(src FEATURE_SRC)

add_executable(feature_server ${FEATURE_SRC})
target_link_libraries(
        feature_server
        feature_protos
        gRPC::grpc++
        gRPC::grpc++_reflection
        glog::glog
        gflags
        redis++::redis++_static
        hiredis::hiredis_static
        uv
        RdKafka::rdkafka++
        ${Boost_LIBRARIES}
)
