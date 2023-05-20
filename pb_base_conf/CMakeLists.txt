cmake_minimum_required(VERSION 3.18)
project(pb_base_conf)

set(CMAKE_CXX_STANDARD 17)

find_package(protobuf REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
include_directories(${protobuf_INCLUDE_DIR})
include_directories(${gflags_INCLUDE_DIR})
include_directories(${glog_INCLUDE_DIR})

add_library(
        pb_schema
        configs/server_config.proto
)
protobuf_generate(TARGET pb_schema LANGUAGE cpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

aux_source_directory(src SRC)
add_executable(server ${SRC} ${PROTO_SRCS})
target_link_libraries(
        server
        pb_schema
        protobuf::libprotobuf
        glog::glog
        gflags
)







