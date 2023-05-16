#include <iostream>
#include <cmath>
#include <cuda_runtime.h>

int main() {
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
        std::cout << cudaGetErrorString(err) << std::endl;
        return 0;
    }
    std::cout << "gpu count: " << count << std::endl;
    for (int i=0; i<count; ++i) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        std::cout << "device number: " << 0 << std::endl;
        std::cout << "name: " << prop.name << std::endl;
        std::cout << "memory: " << prop.totalGlobalMem << std::endl;
        std::cout << "threads per block: " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "compute: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "ms count: " << prop.multiProcessorCount << std::endl;
        std::cout << "thread wrap size: " << prop.warpSize << std::endl;
    }

  return 0;
}
