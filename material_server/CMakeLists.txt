cmake_minimum_required(VERSION 3.18)
project(MaterialServer)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
find_package(libcuckoo REQUIRED)
find_package(absl REQUIRED)

include_directories(${protobuf_INCLUDES})
include_directories(${gRPC_INCLUDES})
include_directories(${glog_INCLUDES})
include_directories(${gflags_INCLUDES})
include_directories(${libcuckoo_INCLUDES})
include_directories(${absl_INCLUDES})
include_directories(${CMAKE_BINARY_DIR})

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_target_property(protoc_location protobuf::protoc LOCATION)

add_library(protos
        protos/account_table.proto
        protos/campaign_table.proto
        protos/creative_table.proto
        protos/material_service.proto
        protos/table.proto
        protos/unit_table.proto
        )

protobuf_generate(TARGET protos LANGUAGE cpp)
protobuf_generate(TARGET protos LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

aux_source_directory(src/service SERVICE_SRC)
add_library(service ${SERVICE_SRC})



add_executable(server src/server.cpp)

target_link_libraries(
        server
        service
        protos
        gRPC::grpc++
        gflags::gflags
        glog::glog
        protobuf::libprotobuf
        libcuckoo::libcuckoo
        absl::random_internal_randen
        absl::random_internal_randen_hwaes
        absl::random_internal_randen_slow
        absl::random_internal_pool_urbg
)

add_executable(client src/client.cpp)


