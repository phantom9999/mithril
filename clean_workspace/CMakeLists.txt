cmake_minimum_required(VERSION 3.18)
project(tmp_workspace)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

aux_source_directory(src SRC)
add_executable(demo ${SRC})
conan_target_link_libraries(demo)
