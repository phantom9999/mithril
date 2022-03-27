cmake_minimum_required(VERSION 3.21)
project(faiss_server)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)


# cmake -DFAISS_OPT_LEVEL=avx2 -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=/home/work/library ..
# ./configure --prefix=/home/work/library

include_directories(/home/work/library/include)
link_directories(/home/work/library/lib)

aux_source_directory(src/builder BUILDER_SRC)
aux_source_directory(src/server SERVER_SRC)

