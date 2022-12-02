cmake_minimum_required(VERSION 3.18)
project(fiber_and_coroutine)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_HOST_UNIX)
    find_package(Protobuf REQUIRED)
    find_package(glog REQUIRED)
    find_package(gflags REQUIRED)
    find_package(gRPC REQUIRED)
    find_package(absl REQUIRED)
    find_package(boost REQUIRED)
endif()
include(cmake/grpc_plugin.cmake)


include_directories(src)

aux_source_directory(src SRC)
add_executable(server ${SRC})






