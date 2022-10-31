cmake_minimum_required(VERSION 3.23)
project(concurrent_lru_cache)

set(CMAKE_CXX_STANDARD 17)

enable_language(ASM)

aux_source_directory(src SRC)
list(APPEND SRC src/hash-crc-x64.S)

add_executable(example ${SRC})

target_link_libraries(
        example
        folly
        tbb
)



