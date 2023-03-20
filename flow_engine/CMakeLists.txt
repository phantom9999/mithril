cmake_minimum_required(VERSION 3.24)
project(flow_engine)

find_package(Boost COMPONENTS fiber log REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
include(../cmake/grpc_generate.cmake)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/framework/graph.proto src/framework/rpc_config.proto src/service.proto)
list(GET PROTO_HDRS 0 PB_FILE)
get_filename_component(PB_PATH ${PB_FILE} DIRECTORY)
include_directories(${PB_PATH})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PB_PATH} src/framework/graph.proto src/service.proto )


aux_source_directory(src/framework FLOW_SRC)
add_executable(flow_engine src/other_ops.cpp src/server.cpp ${FLOW_SRC} ${PROTO_SRCS} ${GRPC_SRCS})
target_link_libraries(
        flow_engine
        ${Boost_LIBRARIES}
        protobuf::libprotobuf
        gRPC::grpc++
)

add_executable(test_graph test/test_graph.cpp src/other_ops.cpp ${FLOW_SRC} ${PROTO_SRCS})
target_link_libraries(test_graph ${Boost_LIBRARIES} protobuf::libprotobuf)
