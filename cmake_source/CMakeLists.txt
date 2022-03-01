cmake_minimum_required(VERSION 3.21)
project(cmake_source)

# https://cmake.org/cmake/help/v3.0/module/ExternalProject.html

include(cmake/remote_gflags.cmake)
include(cmake/remote_c-ares.cmake)

include(cmake/remote_zlib.cmake)
include(cmake/remote_protobuf.cmake)
include(cmake/remote_abseil-cpp.cmake)
include(cmake/remote_grpc.cmake)

message(${PROTOBUF_VERSION})