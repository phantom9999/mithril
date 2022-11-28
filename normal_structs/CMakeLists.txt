cmake_minimum_required(VERSION 3.18)
project(review_coding)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(hashmap HASH_SRC)
add_executable(hash_demo ${HASH_SRC} normal_map/trie_map.cpp normal_map/trie_map.h normal_queue/BlockedQueue.cpp normal_queue/BlockedQueue.h normal_queue/LinkedBlockQueue.cpp normal_queue/LinkedBlockQueue.h)

aux_source_directory(sort_func SORT_SRC)
add_executable(sort_demo ${SORT_SRC})

add_subdirectory(find_parent)



