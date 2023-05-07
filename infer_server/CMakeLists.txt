
set(CMAKE_BUILD_TYPE Debug)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(Torch_ROOT /root/libtorch)
find_package(Torch REQUIRED)
include_directories(${TORCH_INCLUDE_DIRS})
find_package(Boost COMPONENTS filesystem unit_test_framework REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(prometheus-cpp REQUIRED)
#file(GLOB PROTO_FILE proto/tensorflow/core/framework/*.proto proto/tensorflow_serving/apis/*.proto)
#message(${PROTO_FILE})
#list(APPEND PB ${PROTO_FILE})
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

find_library(ONNX_RUNTIME onnxruntime)

if (NOT ONNX_RUNTIME)
    message(FATAL_ERROR "miss onnx-runtime")
endif()

set(COMMIT_HASH "unknown")
set(BRANCH_NAME "master")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
            OUTPUT_VARIABLE COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
            OUTPUT_VARIABLE BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
message(STATUS "Git version is ${BRANCH_NAME}:${COMMIT_HASH}")
configure_file(
        src/version.h.ini
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
        @ONLY
)


protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
        proto/mock.proto
        proto/server_config.proto
        proto/model_spec.proto
        proto/batch_config.proto
        proto/kserve_predict_v2.proto
        proto/data_type.proto
        )
list(GET PROTO_HDRS 0 PB_FILE)
get_filename_component(PB_PATH ${PB_FILE} DIRECTORY)
include_directories(${PB_PATH})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PB_PATH}
        proto/kserve_predict_v2.proto
        )
add_library(schema ${PROTO_SRCS} ${GRPC_SRCS})

include_directories(src)
aux_source_directory(src/batch TORCH_SRC)
aux_source_directory(src/model TORCH_SRC)
aux_source_directory(src/service TORCH_SRC)
aux_source_directory(src/servables TORCH_SRC)
aux_source_directory(src/utils TORCH_SRC)
add_library(service ${TORCH_SRC})

target_link_libraries(
        service
        ${ONNX_RUNTIME}
        ${TORCH_LIBRARIES}
        gRPC::grpc++
        gRPC::grpc++_reflection
        glog::glog
        gflags_static
        ${Protobuf_LIBRARIES}
        Boost::filesystem
        prometheus-cpp::pull
)

add_executable(torch_server src/main.cpp)

target_link_libraries(
        torch_server
        service
        schema
)

add_executable(
        build_warmup
        src/build_warmup.cpp
)
target_link_libraries(
        build_warmup
        schema
        service
        gflags_static
        glog::glog
        ${Protobuf_LIBRARIES}
)


file(GLOB TEST_SOURCES tests/*.cpp)

foreach(testfile ${TEST_SOURCES})
    get_filename_component(filename ${testfile} NAME_WE)
    add_executable(${filename} ${testfile})
    target_link_libraries(
            ${filename}
            service
            schema
            gRPC::grpc++
            glog::glog
            gflags_static
            ${Protobuf_LIBRARIES}
            Boost::filesystem
            Boost::unit_test_framework
            ${TORCH_LIBRARIES}
            prometheus-cpp::pull
    )
endforeach()
