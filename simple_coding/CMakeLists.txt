cmake_minimum_required(VERSION 3.18)
project(review_coding)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(hashmap HASH_SRC)
add_executable(hash_demo ${HASH_SRC} trie_map.cpp trie_map.h)

aux_source_directory(sort_func SORT_SRC)
add_executable(sort_demo ${SORT_SRC})

add_subdirectory(find_parent)



