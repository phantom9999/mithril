cmake_minimum_required(VERSION 3.25)
project(cuda_learning CUDA)

set(CMAKE_CUDA_STANDARD 17)

add_executable(cuda_learning main.cu)

set_target_properties(cuda_learning PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON)
