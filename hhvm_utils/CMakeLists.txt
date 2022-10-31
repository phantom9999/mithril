cmake_minimum_required(VERSION 3.23)
project(hhvm_utils)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src SRC)
aux_source_directory(src/cache SRC)
aux_source_directory(src/portability SRC)
add_library(utils ${SRC})
