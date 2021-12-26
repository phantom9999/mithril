cmake_minimum_required(VERSION 3.18)
project(review_coding)

set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()



aux_source_directory(src SRC)
add_executable(demo ${SRC})
conan_target_link_libraries(demo)

aux_source_directory(src/hashmap HASH_SRC)
add_executable(hash_demo ${HASH_SRC})
conan_target_link_libraries(hash_demo)

aux_source_directory(src/sort_func SORT_SRC)
add_executable(sort_demo ${SORT_SRC})
conan_target_link_libraries(sort_demo)





