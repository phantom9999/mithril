cmake_minimum_required(VERSION 3.18)
project(fiber_and_coroutine)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_HOST_UNIX)
    find_package(Protobuf REQUIRED)
    find_package(glog REQUIRED)
    find_package(gflags COMPONENTS static)
    message(${GFLAGS_TARGET})
    find_package(gRPC REQUIRED)
    find_package(absl REQUIRED)
    find_package(Boost COMPONENTS fiber REQUIRED)
    include(cmake/grpc_plugin.cmake)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/service.proto proto/table.proto)
    list(GET PROTO_HDRS 0 PB_FILE)
    get_filename_component(PB_PATH ${PB_FILE} DIRECTORY)
    grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PB_PATH} proto/service.proto)
elseif(CMAKE_HOST_WIN32)
    include_directories(incs/boost_include)
    include_directories(incs/local_include)
    include_directories(build)
endif()

include_directories(src)
include_directories(${PB_PATH})

aux_source_directory(src SRC)
add_executable(
        meta_server
        ${SRC}
        ${PROTO_SRCS}
        ${GRPC_SRCS}
)

if (CMAKE_HOST_UNIX)
    target_link_libraries(
            meta_server
            gRPC::grpc++
            gRPC::grpc++_reflection
            glog::glog
            gflags
            ${Boost_LIBRARIES}
    )
endif()



