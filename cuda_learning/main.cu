#include <iostream>
#include <cmath>
#include <cuda_runtime.h>

int main() {
    int count = 0;
    cudaGetDeviceCount(&count);
    std::cout << "gpu count: " << count << std::endl;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
        std::cout << cudaGetErrorString(err) << std::endl;
    }
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    std::cout << "device number: " << 0 << std::endl;
    std::cout << "name: " << prop.name << std::endl;
    std::cout << "memory: " << prop.totalGlobalMem << std::endl;
    std::cout << "threads per block: " << prop.maxThreadsPerBlock << std::endl;
  return 0;
}
