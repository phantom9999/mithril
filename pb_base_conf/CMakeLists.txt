cmake_minimum_required(VERSION 3.18)
project(pb_base_conf)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


include_directories(src)

aux_source_directory(src SRC)
aux_source_directory(src/configs SRC)
add_executable(server ${SRC})
conan_target_link_libraries(server)






