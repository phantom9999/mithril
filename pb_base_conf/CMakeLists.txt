cmake_minimum_required(VERSION 3.18)
project(pb_base_conf)

set(CMAKE_CXX_STANDARD 17)
#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()
#conan_target_link_libraries(server)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
find_package(Protobuf REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
include_directories(${protobuf_INCLUDE_DIR})
include_directories(${gflags_INCLUDE_DIR})
include_directories(${glog_INCLUDE_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/configs/server_config.proto)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/src)
include_directories(src)

aux_source_directory(src SRC)
add_executable(server ${SRC} ${PROTO_SRCS})
target_link_libraries(
        server
        protobuf::libprotobuf
        glog::glog
        gflags::gflags
)







