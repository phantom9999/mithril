cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(Protobuf REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(absl REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(faiss REQUIRED)
find_package(gRPC REQUIRED)
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_target_property(protoc_location protobuf::protoc LOCATION)

include_directories(${faiss_INCLUDES})
include_directories(${Boost_INCLUDES})
include_directories(${protobuf_INCLUDES})
include_directories(${gRPC_INCLUDES})
include_directories(${gflags_INCLUDES})
include_directories(${glog_INCLUDES})
include_directories(${absl_INCLUDES})

include_directories(${CMAKE_BINARY_DIR})

include_directories(src)

add_library(faiss_proto
    proto/builder_config.proto
    proto/index_constants.proto
    proto/service.proto
    proto/source.proto
    proto/server_config.proto
    )
protobuf_generate(TARGET faiss_proto LANGUAGE cpp APPEND_PATH .)
protobuf_generate(TARGET faiss_proto LANGUAGE grpc APPEND_PATH . GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

target_link_libraries(faiss_proto protobuf::libprotobuf)


aux_source_directory(src/common CONFIG_SRC)
add_library(common ${CONFIG_SRC})

target_link_libraries(common Boost::filesystem absl::strings)

aux_source_directory(src/builder BUILDER_SRC)
add_executable(faiss_builder ${BUILDER_SRC})

add_executable(data_gen src/data_gen/data_gen.cpp)

aux_source_directory(src/server SERVER_SRC)
add_executable(faiss_server ${SERVER_SRC})

add_executable(client src/client/client.cpp)

target_link_libraries(
    data_gen
    common
    faiss_proto
    absl::strings
    gflags::gflags
)
target_link_libraries(
    faiss_builder
    common
    faiss_proto
    gflags::gflags
    absl::strings
    faiss
    glog::glog
)

target_link_libraries(
    faiss_server
    faiss_proto
    common
    gRPC::grpc++
    glog::glog
    gflags::gflags
    protobuf::libprotobuf
    faiss
)

target_link_libraries(
    client
    faiss_proto
    gRPC::grpc++
    glog::glog
    protobuf::libprotobuf
)
