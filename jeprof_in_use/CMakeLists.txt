cmake_minimum_required(VERSION 3.18)
project(jeprof_in_use)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

aux_source_directory(src SRC)
add_executable(server ${SRC})
conan_target_link_libraries(server)






