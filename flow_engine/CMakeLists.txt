cmake_minimum_required(VERSION 3.22)
project(flow_engine)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

find_package(Boost COMPONENTS fiber log REQUIRED)
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_target_property(protoc_location protobuf::protoc LOCATION)

add_library(
        pb_schema
        protos/graph.proto
        protos/rpc_config.proto
        protos/service.proto
)
protobuf_generate(TARGET pb_schema LANGUAGE cpp)
protobuf_generate(TARGET pb_schema LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")
include_directories(${CMAKE_CURRENT_BINARY_DIR})


aux_source_directory(src/framework FLOW_SRC)
add_executable(flow_engine src/other_ops.cpp src/server.cpp ${FLOW_SRC})
target_link_libraries(
        flow_engine
        pb_schema
        ${Boost_LIBRARIES}
        protobuf::libprotobuf
        gRPC::grpc++
)

add_executable(test_graph test/test_graph.cpp src/other_ops.cpp ${FLOW_SRC})
target_link_libraries(
        test_graph
        pb_schema
        ${Boost_LIBRARIES}
        protobuf::libprotobuf
        gRPC::grpc++
)
