cmake_minimum_required(VERSION 3.18)
project(strategy_server)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(third/better-enums)


aux_source_directory(proto SRC_PROTO)
include_directories(proto)
add_library(service_proto ${SRC_PROTO})

include_directories(src)

# 服务端
aux_source_directory(src SRC)
aux_source_directory(src/strategies SRC)
add_executable(strategy_server ${SRC})
target_link_libraries(strategy_server service_proto)
conan_target_link_libraries(strategy_server)

# 客户端
aux_source_directory(src/client SRC_CLIENT)
add_executable(client ${SRC_CLIENT})
target_link_libraries(client service_proto)
conan_target_link_libraries(client)

