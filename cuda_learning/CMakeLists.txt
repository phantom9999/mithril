cmake_minimum_required(VERSION 3.22)
project(cuda_learning CUDA)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES "86")
set(CMAKE_CUDA_COMPILER "/usr/local/cuda/bin/nvcc")


add_executable(cuda_learning main.cu)

set_target_properties(cuda_learning PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)
