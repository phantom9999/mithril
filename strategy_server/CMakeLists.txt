cmake_minimum_required(VERSION 3.18)
project(strategy_server)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Taskflow REQUIRED)

include_directories(${Taskflow_INCLUDES})
include_directories(${protobuf_INCLUDES})

include_directories(third/better-enums)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/service.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/graph.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(service_proto ${PROTO_SRCS})

include_directories(src)

# 服务端
aux_source_directory(src SRC)
aux_source_directory(src/strategies SRC)
add_executable(strategy_server ${SRC})
target_link_libraries(strategy_server service_proto)
#conan_target_link_libraries(strategy_server)

# 客户端
aux_source_directory(src/client SRC_CLIENT)
add_executable(client ${SRC_CLIENT})
target_link_libraries(client service_proto)
#conan_target_link_libraries(client)

